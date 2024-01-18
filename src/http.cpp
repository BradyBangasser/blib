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

    int *sock = initSock(urlInfo->addrInfo);

    if (sock == NULL) {
        freeHttpMsg(httpMsg);
        freeUrlInfo(urlInfo);
        cleanupWindows();
        return NULL;
    }

    result = connect(*sock, urlInfo->addrInfo->ai_addr, urlInfo->addrInfo->ai_addrlen);
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
        initSSL(sock, &ssl, &ctx);

        if (ssl == NULL || ctx == NULL) {
            freeHttpMsg(httpMsg);
            freeUrlInfo(urlInfo);
            cleanupWindows();
            return NULL;
        }

        printf("%x this 2\n", ssl);

        result = SSL_connect(ssl);

        printf("Connected\n");

        SSL_load_error_strings();

        char msg[256];

        if (result != 1) {
            ERR_error_string(SSL_get_error(ssl, result), msg);
        printf("%s %i\n", msg, result);
            freeHttpMsg(httpMsg);
            freeUrlInfo(urlInfo);
            cleanupWindows();
            return NULL;
        }

        freeUrlInfo(urlInfo);

        printf("here\n");

        result = SSL_write(ssl, httpMsg, strlen(httpMsg));

        printf("%lli\n", result);

        result = SSL_read(ssl, receiveBuffer, RECEIVE_BUFFER_SIZE);

        printf("%s\n", receiveBuffer);

        cleanupSSL(ssl, ctx);
    } else {
        printf("here\n");
        freeUrlInfo(urlInfo);
    }

    cleanupSock(sock);

    freeHttpMsg(httpMsg);

    cleanupWindows();

    return NULL;
}