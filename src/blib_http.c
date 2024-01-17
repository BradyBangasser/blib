#include "blib_http.h"
#include "blib_constants.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#else
#include <ws2tcpip.h>
#include <winsock2.h>
#endif

#include <openssl/types.h>
#include <openssl/ssl.h>

int *initSock(struct addrinfo *addr) {
    // This deals with windows being special
    int *sock = malloc(sizeof(int));

    *sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

    if (*sock == -1) return NULL;
    return sock;
}


void cleanupSock(int *sock) {
    shutdown(*sock, 0x02);

    if (sock != NULL) {
        #ifndef _WIN32
        close(*sock);
        #else
        closesocket(*sock);
        #endif
        free(sock);
    }
}

void initSSL(int *sock, SSL **ssl, SSL_CTX **ctx) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    *ctx = SSL_CTX_new(SSLv23_client_method());
    *ssl = SSL_new(*ctx);

    int result = SSL_set_fd(*ssl, *sock);

    if (result != 1) *ssl = NULL;
}

void cleanupSSL(SSL *ssl, SSL_CTX *ctx) {
    if (ctx != NULL) {
        SSL_CTX_free(ctx);
        ctx = NULL;
    }

    if (ssl != NULL) {
        SSL_free(ssl);
        ssl = NULL;
    }
}

void createHttpMsg(char **buf, const char *method, const char *path, const char *host, struct Blib_Header *headers, size_t headersLen, const char *msg) {
    // This will dynamically allocate memory for the http message
    int i;
    char *curs = NULL, *headersString = NULL;
    const char *currentHeader;

    if (headersLen > 0 && headers != NULL) {
        currentHeader = createHeaderString(headers);
        // Allocs the size of the first header, plus the null character
        headersString = realloc(headersString, (1 + strlen(currentHeader)) * sizeof(char));

        if (curs == NULL) {
            // cry
            *buf = NULL;
        }
        
        // sprints the headers, and the null character to the string
        snprintf(headersString, strlen(currentHeader) + 1, currentHeader);

        freeHeaderString(currentHeader);

        if (headersLen > 1) {
            for (i = 1; i < headersLen; i++) {
                currentHeader = createHeaderString(headers + i);
                // allocs the strlen of the entire header string up to this point plus the size of the current header and a null character
                headersString = (char *) realloc(headersString, (strlen(headersString) + strlen(currentHeader) + 1) * sizeof(char));

                // Starts writing at the previous headers null character
                if (headersString != NULL) curs = headersString + strlen(headersString);
                else {
                    // cry
                    *buf = NULL;
                }

                snprintf(curs, strlen(currentHeader) + 1, currentHeader);
                freeHeaderString(currentHeader);
            }
        }
    }

    char *baseString = "%s %s HTTP/%s\r\n%sHost: %s\r\n\r\n";
    size_t msgLen = snprintf(NULL, 0, baseString, method, path, BLIB_HTTP_VERSION, headersString, host) + 2;
    char *httpMsg = (char *) malloc(msgLen * sizeof(char));
    snprintf(httpMsg, msgLen, baseString, method, path, BLIB_HTTP_VERSION, headersString, host);
    free(headersString);
    *buf = httpMsg;
}

void freeHttpMsg(char *msg) {
    if (msg != NULL) {
        free(msg);
        msg = NULL;
    }
}

struct addrinfo *blibGetAddrInfo(const char *host, const char *port) {
    struct addrinfo hints, *addr;

    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    memset(&hints, 0, sizeof hints);

    int result = getaddrinfo(host, port, &hints, &addr);

    if (result != 0) {
        return NULL;
    } else {
        return addr;
    }
}

void blibFreeAddrInfo(struct addrinfo *addrinfo) {
    if (addrinfo != NULL) {
        freeaddrinfo(addrinfo);
        addrinfo = NULL;
    }
}

struct UrlInfo *getUrlInfo(const char *url) {
    size_t len;
    char *tok, *str;

    struct UrlInfo *urlInfo = malloc(sizeof(struct UrlInfo));
    if (urlInfo == NULL) return NULL;

    len = strlen(url) + 1;
    urlInfo->url = (char *) malloc(sizeof(char) * len);

    if (urlInfo->url == NULL) {
        freeUrlInfo(urlInfo);
        return NULL;
    }

    snprintf(urlInfo->url, len, url);

    tok = strstr(urlInfo->url, "://");

    len = tok - urlInfo->url + 1;
    urlInfo->proto = (char *) malloc(sizeof(char) * len);
    if (urlInfo->proto == NULL) {
        freeUrlInfo(urlInfo);
        return NULL;
    }
    snprintf(urlInfo->proto, len, urlInfo->url);

    // Get the host
    tok += 3;
    str = tok;
    tok = strstr(tok, "/");
    
    if (tok == NULL) {
        tok = strstr(str, "?");
        if (tok == NULL) {
            tok = str + strlen(str);
        }
    }

    len = tok - str + 1;

    urlInfo->host = (char *) malloc(sizeof(char) * len);
    if (urlInfo->host == NULL) {
        freeUrlInfo(urlInfo);
        return NULL;
    }

    snprintf(urlInfo->host, len, str);

    // Path
    len = strlen(tok) + 1;

    if (len < 2) {
        len = 2;
        tok--;
        tok = "/";
    }

    if (tok[0] != '/') {
        tok--;
        len++;
        tok[0] = '/';
    }

    urlInfo->path = (char *) malloc(sizeof(char) * len);
    if (urlInfo->path == NULL) {
        freeUrlInfo(urlInfo);
        return NULL;
    }

    snprintf(urlInfo->path, len, tok);

    urlInfo->addrInfo = blibGetAddrInfo(urlInfo->host, (strcmp(urlInfo->proto, "http") ? HTTP_PORT : HTTPS_PORT));
    if (urlInfo->addrInfo == NULL) {
        freeUrlInfo(urlInfo);
        return NULL;
    }

    return urlInfo;
}

void freeUrlInfo(struct UrlInfo *urlInfo) {
    if (urlInfo != NULL) {
        if (urlInfo->proto != NULL) free((char *) urlInfo->proto);
        if (urlInfo->host != NULL) free((char *) urlInfo->host);
        if (urlInfo->path != NULL) free((char *) urlInfo->path);
        if (urlInfo->url != NULL) free((char *) urlInfo->url);
        if (urlInfo->addrInfo != NULL) blibFreeAddrInfo(urlInfo->addrInfo);
        free(urlInfo);
    }
}

const char *createHeaderString(struct Blib_Header *header) {
    const char *baseString = "%s: %s\r\n";
    size_t headerStringLen = snprintf(NULL, 0, baseString, header->name, header->value) + 2;
    char *headerString = (char *) malloc(headerStringLen * sizeof(char));
    snprintf(headerString, headerStringLen, baseString, header->name, header->value);
    return headerString;
}

void freeHeaderString(const char *headerString) {
    if (headerString != NULL) {
        free((void *) headerString);
    }
}