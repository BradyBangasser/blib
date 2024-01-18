#include "http.hpp"
#include "blib_http.h"

#include <string>
#include <stdio.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BLIB_HEADER(n, v) { n, v, (strlen(n) + strlen(v)) }

#define RECEIVE_BUFFER_SIZE 4048

struct Blib_Response *request(std::string url, struct RequestOptions *opts) {
    size_t result;

    if (opts == NULL) {
        struct RequestOptions options;
        opts = &options;
    }

    initWindows();
    struct UrlInfo *urlInfo = getUrlInfo(url.c_str());

    struct Blib_Header headers[50] = {
        BLIB_HEADER("coc", "ball"),
        BLIB_HEADER("thy", "cars0"),
        BLIB_HEADER("th", "cars1"),
        BLIB_HEADER("thy", "cars2"),
        BLIB_HEADER("thy", "cars3"),
        BLIB_HEADER("thy", "cars4"),
        BLIB_HEADER("thy", "cars5"),
        BLIB_HEADER("thy", "cars6"),
        BLIB_HEADER("thy", "cars7"),
        BLIB_HEADER("thy", "car8"),
    };

    SSL_library_init();
    OpenSSL_add_ssl_algorithms();

    char *httpMsg = NULL;
    createHttpMsg(&httpMsg, opts->method, urlInfo->path, urlInfo->host, headers, 10, "fdgadfgadfgsadfg");

    int sock = initSock(urlInfo->addrInfo);
    printf("%i\n", sock);

    if (sock == -1) {
        freeHttpMsg(httpMsg);
        freeUrlInfo(urlInfo);
        cleanupWindows();
        return NULL;
    }

    result = connect(sock, urlInfo->addrInfo->ai_addr, urlInfo->addrInfo->ai_addrlen);
    printf("%i this\n", result);

    char receiveBuffer[RECEIVE_BUFFER_SIZE];

    if (result != 0) {
        freeHttpMsg(httpMsg);
        freeUrlInfo(urlInfo);
        cleanupWindows();
        return NULL;
    }

    if (opts->secure) {
        SSL *ssl = NULL;
        SSL_CTX *ctx = NULL;

        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();

        ctx = SSL_CTX_new(SSLv23_client_method());
        ssl = SSL_new(ctx);

        result = SSL_set_fd(ssl, sock);
        if (result != 1) {
            printf("here\n");
        }

        result = SSL_connect(ssl);

        if (result != 1) {
            printf("%i res\n", result);
            freeHttpMsg(httpMsg);
            freeUrlInfo(urlInfo);
            cleanupWindows();
            return NULL;
        }

        freeUrlInfo(urlInfo);

        printf("here\n");
        result = SSL_write(ssl, httpMsg, strlen(httpMsg));

        result = SSL_read(ssl, receiveBuffer, RECEIVE_BUFFER_SIZE);

        cleanupSSL(ssl, ctx);
    } else {
        result = send(sock, httpMsg, strlen(httpMsg), 0);

        printf("http res: %i\n", result);

        result = recv(sock, receiveBuffer, RECEIVE_BUFFER_SIZE, 0);

        printf("%s\n", receiveBuffer);

        freeUrlInfo(urlInfo);
    }

    cleanupSock(sock);

    freeHttpMsg(httpMsg);

    cleanupWindows();

    return NULL;
}