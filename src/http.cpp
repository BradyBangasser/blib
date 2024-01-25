#include "http.hpp"
#include "blib_http.h"
#include "blib_constants.h"

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#endif

#include <string>
#include <stdio.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


#define RECEIVE_BUFFER_SIZE 4096

using namespace blib_http;

// Implement error handling and make the request fns wrappers of _request fn that hold all logic
/*
    Internal implemention of the request functions
    If writeOutToFile is false, all data will be stored in the out param, else the out param will be used as a file path to write to
*/
int _request(const std::string url, const struct RequestOptions *opts, std::string &out, bool writeOutToFile) {
    size_t result;
    char *httpMsg;
    FILE *f;
    std::string content;
    struct addrinfo *addr;
    SSL *ssl = NULL;
    SSL_CTX *ctx = NULL;

    char receiveBuffer[RECEIVE_BUFFER_SIZE];
    memset(receiveBuffer, '\0', sizeof(receiveBuffer));


    // Likely will sigsegv
    if (opts == NULL) {
        struct RequestOptions options;
        opts = &options;
    }
    initWindows();
    struct UrlInfo *urlInfo = getUrlInfo(url.c_str());

    bool headersExist = opts->headers != NULL;
    createHttpMsg(&httpMsg, opts->method, urlInfo->path, urlInfo->host, (headersExist ? opts->headers->data() : NULL), (headersExist ? opts->headers->size() : 0), "");

    BINFO("Creating Socket\n");
    int sock = initSock(urlInfo->addrInfo);

    if (sock == -1) {
        BERROR("Error initizing socket, errno: %i\n", getErrorCode());
        freeHttpMsg(&httpMsg);
        freeUrlInfo(&urlInfo);
        cleanupWindows();
        return error_codes::FAILURE_TO_INIT_SOCKET;
    }
    BINFO("Successfully Created Socket\n");

    BINFO("Attempting to Connect to %s\n", urlInfo->host);
    addr = urlInfo->addrInfo;
    while (addr != NULL) {
        result = connect(sock, urlInfo->addrInfo->ai_addr, urlInfo->addrInfo->ai_addrlen);
        if (result == 0) {
            BINFO("Connected Successfully\n");
            break;
        } else {
            BINFO("Failed to connect, attempting next possible address\n");
            addr = addr->ai_next;
        }
    }

    if (result != 0) {
        BERROR("Failed to connect to socket, errno: %i\n", errno);
        freeHttpMsg(&httpMsg);
        freeUrlInfo(&urlInfo);
        cleanupWindows();
        return error_codes::FAILURE_TO_CONNECT;
    }

    // if the out param will be used for storage clear it
    if (!writeOutToFile) out.clear();
    else f = fopen(out.c_str(), "wb");
    if (f == NULL) {
        BERROR("Error opening file: %s, errno: %i\n", out.c_str(), errno);
        freeHttpMsg(&httpMsg);
        freeUrlInfo(&urlInfo);
        cleanupWindows();
        return error_codes::FAILURE_TO_OPEN_OUT_FILE;
    }


    if (opts->secure) {
        BINFO("Attempting to Init SSL\n");
        result = initSSL(sock, &ssl, &ctx);

        if (ssl == NULL || ctx == NULL) {
            BERROR("Failed to init SSL, result: %lli, error code: %i\n", result, getErrorCode());
            freeHttpMsg(&httpMsg);
            freeUrlInfo(&urlInfo);
            cleanupWindows();
            if (writeOutToFile) fclose(f);
            return error_codes::FAILURE_TO_INIT_SSL;
        }

        BINFO("Success\n");

        BINFO("Attempting to connect SSL socket\n");
        result = SSL_connect(ssl);

        if (result != 1) {
            BERROR("Error connecting SSL socket, result: %lli, error code: %i\n", result, SSL_get_error(ssl, result));
            freeHttpMsg(&httpMsg);
            freeUrlInfo(&urlInfo);
            cleanupWindows();
            if (writeOutToFile) fclose(f);
            return error_codes::SSL_FAILURE_TO_CONNECT;
        }

        BINFO("Success\n");

        BINFO("Attempting to send msg over SSL\n");
        result = SSL_write(ssl, httpMsg, strlen(httpMsg));

        if (result <= 0) {
            // Eventually add retry write
            BERROR("Error writing data: %i\n", SSL_get_error(ssl, result));

            freeUrlInfo(&urlInfo);
            cleanupSSL(&ssl, &ctx);
            cleanupSock(sock);
            if (writeOutToFile) fclose(f);
            freeHttpMsg(&httpMsg);
            cleanupWindows();

            return error_codes::SSL_FAILURE_TO_WRITE_DATA;
        }

        BINFO("Success\n");
    } else {
        BINFO("Attempting to send data\n");
        result = send(sock, httpMsg, strlen(httpMsg), 0);

        if (result == -1) {
            int err;

            #if defined(_WIN32) || defined(_WIN64)
                err = WSAGetLastError();
            #else
                err = errno;
            #endif

            freeUrlInfo(&urlInfo);
            cleanupSSL(&ssl, &ctx);
            cleanupSock(sock);
            if (writeOutToFile) fclose(f);
            freeHttpMsg(&httpMsg);
            cleanupWindows();

            BERROR("Error sending data: %i\n", err);

            return error_codes::FAILURE_TO_WRITE_DATA;
        }

        BINFO("Success\n");
    }

    freeUrlInfo(&urlInfo);
    freeHttpMsg(&httpMsg);

    BINFO("Starting read\n");
    if (opts->secure) {
        // Implement timeouts
        BINFO("SSL reading\n");
        result = SSL_read(ssl, receiveBuffer, RECEIVE_BUFFER_SIZE);

        if (result <= 0) {
            BERROR("Error reading data from SSL socket: %i\n", SSL_get_error(ssl, result));
            cleanupSSL(&ssl, &ctx);
            cleanupSock(sock);
            if (writeOutToFile) fclose(f);
            cleanupWindows();
            return error_codes::SSL_FAILURE_TO_READ;
        }
    } else {
        BINFO("Reading\n");
        result = recv(sock, receiveBuffer, RECEIVE_BUFFER_SIZE, 0);

        if (result <= 0) {
            BERROR("Error reading data from socket: %i\n", SSL_get_error(ssl, result));
            cleanupSock(sock);
            if (writeOutToFile) fclose(f);
            cleanupWindows();
            return error_codes::FAILURE_TO_READ;
        }
    }

    BINFO("Successfully read first part\n");

    char *contentStart = strstr(receiveBuffer, "\r\n\r\n");
    char *header;

    if (contentStart == NULL) {
        BINFO("Header not complete in first chunk...\n");

        char *headerBuf;
        headerBuf = (char *) malloc((strlen(receiveBuffer) + 1) * sizeof(char));

        if (headerBuf == NULL) {
            BERROR("Failure to allocate mem for the header\n");
            cleanupSSL(&ssl, &ctx);
            cleanupSock(sock);
            cleanupWindows();
            if (writeOutToFile) fclose(f);
            return error_codes::FAILURE_TO_ALLOC_MEM;
        }

        memcpy_s(headerBuf, strlen(receiveBuffer) + 1, receiveBuffer, strlen(receiveBuffer) + 1);

        do {
            if (opts->secure) {
                result = SSL_read(ssl, receiveBuffer, RECEIVE_BUFFER_SIZE);
            } else {
                result = recv(sock, receiveBuffer, RECEIVE_BUFFER_SIZE, 0);
            }
            headerBuf = (char *) realloc(headerBuf, (strlen(headerBuf) + 1 + result) * sizeof(char));
            strncpy(headerBuf + strlen(headerBuf), receiveBuffer, result);
        } while (contentStart = strstr(receiveBuffer, "\r\n\r\n"), contentStart == NULL);

        contentStart = strstr(headerBuf, "\r\n\r\n");

        size_t headerLen = contentStart - headerBuf + 4;

        header = (char *) malloc(headerLen * sizeof(char));
        strncpy(header, headerBuf, headerLen);

        free(headerBuf);
    } else {
        contentStart += 4;
        header = (char *) malloc((contentStart - receiveBuffer) * sizeof(char));
        strncpy(header, receiveBuffer, contentStart - receiveBuffer);
    }

    struct Blib_Response_Header *headStruct = parseResponseHeader(header);
    free(header);

    if (opts->outFile == NULL) {
        content.append(contentStart);
    } else {
        fwrite(contentStart, sizeof(char), strlen(contentStart), f);
    }

    do {
        if (opts->secure) {
            result = SSL_read(ssl, receiveBuffer, RECEIVE_BUFFER_SIZE);
        } else {
            result = recv(sock, receiveBuffer, RECEIVE_BUFFER_SIZE, 0);
        }

        if (result == 0) break;
    
        if (opts->outFile == NULL) {
            content.append(receiveBuffer);
        } else {
            fwrite(receiveBuffer, sizeof(char), strlen(receiveBuffer), f);
        }
    } while (strstr(receiveBuffer, "\r\n\r\n") == NULL);
    
    if (opts->outFile != NULL) {
        fclose(f);
    }

    if (opts->secure) {
        cleanupSSL(&ssl, &ctx);
    }

    freeResponseHeader(&headStruct);

    cleanupSock(sock);

    cleanupWindows();

    return NULL;
}

int request(const std::string url, const std::string outFile, struct RequestOptions *opts) {
}