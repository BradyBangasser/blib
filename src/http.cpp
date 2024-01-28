#include "http.hpp"
#include "blib_http.h"
#include "blib_constants.h"

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#endif

#include <string>
#include <stdio.h>
#include <string.h>
#include <openssl/types.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdlib.h>
#include <string.h>

#define RECEIVE_BUFFER_SIZE 4096

using namespace blib_http;

// Implement error handling and make the request fns wrappers of _request fn that hold all logic
/*
    Internal implemention of the request functions
    If writeOutToFile is false, all data will be stored in the out param, else the out param will be used as a file path to write to
*/
int _request(const std::string url, const struct RequestOptions *opts, std::string &out, bool writeOutToFile) {
    BINFO("Starting request to %s\n", url.c_str());

    int64_t result;
    char *httpMsg = NULL;
    FILE *f = NULL;
    std::string content;
    struct addrinfo *addr = NULL;
    SSL *ssl = NULL;
    SSL_CTX *ctx = NULL;
    SSL_CTX *ctx_copy = NULL;

    // This feels like a bad practice
    bool freeNeeded = false;
    bool secure;

    // this allows for setting the defaults if the user supplied NULL
    struct RequestOptions *m_opts = const_cast<struct RequestOptions *>(opts);

    char receiveBuffer[RECEIVE_BUFFER_SIZE];
    memset(receiveBuffer, '\0', sizeof(receiveBuffer));


    // Only malloc if needed
    if (m_opts == NULL) {
        freeNeeded = true;
        m_opts = (struct RequestOptions *) malloc(sizeof(struct RequestOptions));

        if (m_opts == NULL) {
            BERROR("Unable to allocate mem for options\n");
            return error_codes::FAILURE_TO_ALLOC_MEM;
        }

        struct RequestOptions options;
        
        memcpy((void *) m_opts, &options, sizeof(struct RequestOptions));
    }

    initWindows();

    BINFO("Getting url info");
    struct UrlInfo *urlInfo = getUrlInfo(url.c_str());

    if (urlInfo == NULL) {
        BERROR("Failed to get URL Info\n");
        if (freeNeeded) free(m_opts);
        return error_codes::FAILURE_TO_GET_URL_INFO;
    } else {
        secure = strcmp(urlInfo->proto, "http") != 0;
    }

    bool headersExist = m_opts->headers != NULL;
    BINFO("Getting Http message\n");
    createHttpMsg(&httpMsg, m_opts->method, urlInfo->path, urlInfo->host, (headersExist ? m_opts->headers->data() : NULL), (headersExist ? m_opts->headers->size() : 0), "");

    if (httpMsg == NULL) {
        BERROR("Failed to create Http message\n");
        if (freeNeeded) free(m_opts);
        freeUrlInfo(&urlInfo);
        return error_codes::FAILURE_TO_CREATE_HTTP_MESSAGE;
    }

    BINFO("Creating Socket\n");
    int sock = initSock(urlInfo->addrInfo);

    if (sock == -1) {
        BERROR("Error initizing socket, errno: %i\n", getErrorCode());
        freeHttpMsg(&httpMsg);
        freeUrlInfo(&urlInfo);
        cleanupWindows();
        if (freeNeeded) free(m_opts);
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
        if (freeNeeded) free(m_opts);
        return error_codes::FAILURE_TO_CONNECT;
    }

    // if the out param will be used for storage clear it
    if (!writeOutToFile) out.clear();
    else f = fopen(out.c_str(), "wb");
    if (writeOutToFile && f == NULL) {
        BERROR("Error opening file: %s, errno: %i\n", out.c_str(), errno);
        freeHttpMsg(&httpMsg);
        freeUrlInfo(&urlInfo);
        cleanupWindows();
        if (freeNeeded) free(m_opts);
        return error_codes::FAILURE_TO_OPEN_OUT_FILE;
    }


    if (secure) {
        BINFO("Attempting to Init SSL\n");
        result = initSSL(sock, &ssl, &ctx);

        printf("%x %x here\n", ssl, ctx);
        ctx_copy = ctx;

        if (ssl == NULL || ctx == NULL) {
            BERROR("Failed to init SSL, result: %lli, error code: %i\n", result, getErrorCode());
            freeHttpMsg(&httpMsg);
            freeUrlInfo(&urlInfo);
            if (freeNeeded) free(m_opts);
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
            if (freeNeeded) free(m_opts);
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
            if (freeNeeded) free(m_opts);
            if (writeOutToFile) fclose(f);
            freeHttpMsg(&httpMsg);
            cleanupWindows();

            return error_codes::SSL_FAILURE_TO_WRITE_DATA;
        }

        printf("%x %x\n", ssl, ctx);
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
            if (freeNeeded) free(m_opts);
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
    if (secure) {
        // Implement timeouts
        BINFO("SSL reading\n");
        result = SSL_read(ssl, receiveBuffer, RECEIVE_BUFFER_SIZE);

        if (result <= 0) {
            BERROR("Error reading data from SSL socket: %i\n", SSL_get_error(ssl, result));
            cleanupSSL(&ssl, &ctx);
            cleanupSock(sock);
            if (freeNeeded) free(m_opts);
            if (writeOutToFile) fclose(f);
            cleanupWindows();
            return error_codes::SSL_FAILURE_TO_READ;
        }
    } else {
        BINFO("Reading\n");
        result = recv(sock, receiveBuffer, RECEIVE_BUFFER_SIZE, 0);

        if (result <= 0) {
            BERROR("Error reading data from socket: %i\n", errno);
            cleanupSock(sock);
            if (writeOutToFile) fclose(f);
            if (freeNeeded) free(m_opts);
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
            if (freeNeeded) free(m_opts);
            if (writeOutToFile) fclose(f);
            return error_codes::FAILURE_TO_ALLOC_MEM;
        }

        // probably requires error handling and checking
        memcpy(headerBuf, receiveBuffer, strlen(receiveBuffer) + 1);

        do {
            BINFO("Attempting to fetch the rest if the header\n");
            if (secure) {
                result = SSL_read(ssl, receiveBuffer, RECEIVE_BUFFER_SIZE);
            } else {
                result = recv(sock, receiveBuffer, RECEIVE_BUFFER_SIZE, 0);
            }

            if (result <= 0) {
                BERROR("Bad Response, header not complete\n");
                free(headerBuf);
                cleanupSSL(&ssl, &ctx);
                cleanupSock(sock);
                if (freeNeeded) free(m_opts);
                cleanupWindows();
                return error_codes::BAD_RESPONSE_NO_HEADER;
            }

            headerBuf = (char *) realloc(headerBuf, (strlen(headerBuf) + 1 + result) * sizeof(char));

            if (headerBuf == NULL) {
                BERROR("Failed to allocate mem for headerBuf\n");
                cleanupSSL(&ssl, &ctx);
                cleanupSock(sock);
                cleanupWindows();
                if (freeNeeded) free(m_opts);
                return error_codes::FAILURE_TO_ALLOC_MEM;
            }

            strncpy(headerBuf + strlen(headerBuf), receiveBuffer, result);
        } while (contentStart = strstr(receiveBuffer, "\r\n\r\n"), contentStart == NULL);

        BINFO("Assembling header\n");

        contentStart = strstr(headerBuf, "\r\n\r\n");

        size_t headerLen = contentStart - headerBuf + 4;

        header = (char *) malloc(headerLen * sizeof(char));

        if (header == NULL) {
            BERROR("Failed to allocate mem for header\n");
            free(headerBuf);
            cleanupSSL(&ssl, &ctx);
            cleanupSock(sock);
            cleanupWindows();
            if (freeNeeded) free(m_opts);
            return error_codes::FAILURE_TO_ALLOC_MEM;
        }

        strncpy(header, headerBuf, headerLen);

        free(headerBuf);
    } else {
        contentStart += 4;
        header = (char *) malloc((contentStart - receiveBuffer) * sizeof(char) + 1);

        if (header == NULL) {
                BERROR("Failed to allocate mem for header\n");
                cleanupSSL(&ssl, &ctx);
                cleanupSock(sock);
                if (freeNeeded) free(m_opts);
                cleanupWindows();
                return error_codes::FAILURE_TO_ALLOC_MEM;
        }

        strncpy(header, receiveBuffer, contentStart - receiveBuffer);
        header[contentStart - receiveBuffer] = '\0';
    }

    BINFO("Attempting to parse header\n");

    struct Blib_Response_Header *headStruct = parseResponseHeader(header);

    if (headStruct == NULL) {
        BERROR("Failed to parse header\n");
        free(header);
        cleanupSSL(&ssl, &ctx);
        cleanupSock(sock);
        cleanupWindows();
        if (freeNeeded) free(m_opts);
        return error_codes::BAD_RESPONSE_FAILED_TO_PARSE_HEADER;
    }


    free(header);

    if (writeOutToFile) {
        // BINFO("Writing content to %s\n", out.c_str());
        fwrite(contentStart, sizeof(char), strlen(contentStart), f);
    } else {
        content.append(contentStart);
    }

    do {
        if (secure) {
            // BINFO("Reading data from SSL\n");
            result = SSL_read(ssl, receiveBuffer, RECEIVE_BUFFER_SIZE);
        } else {
            BINFO("Reading data\n");
            result = recv(sock, receiveBuffer, RECEIVE_BUFFER_SIZE, 0);
        }

        // BINFO("Read %zu bytes\n", result);
            printf("Pre: %x\n", ctx);

        if (result == 0) {
            break;
        }

        receiveBuffer[result] = '\0';
    
        if (writeOutToFile) {
            // BINFO("Writing %zu bytes to %s\n", result, out.c_str());
            fwrite(receiveBuffer, sizeof(char), strlen(receiveBuffer), f);
        } else {
            content.append(receiveBuffer);
        }
            printf("Pos: %x\n", ctx);
    } while (strstr(receiveBuffer, "\r\n\r\n") == NULL);
    
    if (writeOutToFile) {
        fclose(f);
    }

    printf("Freeing ctx %i\n", (uint64_t)ctx - (uint64_t)ctx_copy);
    cleanupSSL(&ssl, &ctx);
    printf("This\n");

    if (!writeOutToFile) out = content;
    printf("here\n");

    freeResponseHeader(&headStruct);

    cleanupSock(sock);

    cleanupWindows();

    if (freeNeeded) free(m_opts);

    return 0;
}

template<> const std::string blib_http::request<const std::string>(const std::string url, const struct RequestOptions *opts) {
    std::string content;
    int result = _request(url, opts, content, false);

    if (result != 0) {
        struct RequestError err {
            result
        };

        throw err;
    }

    return content;
}

template<> int blib_http::request<int>(const std::string url, const std::string outfile, const struct RequestOptions *opts) {
    // This is to make sure that the const val is not modified
    std::string fname = outfile;   

    int result = _request(url, opts, fname, true);

    if (result != 0) {
        struct RequestError err {
            result
        };

        throw err;
    } 

    return 0;
}