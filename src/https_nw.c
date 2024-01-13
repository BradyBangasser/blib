#include "https_nw.h"
#include "blib_http.h"
#include "blib_constants.h"

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

static SSL_CTX *sslCtx = NULL;
static SSL *ssl = NULL;
static struct addrinfo *hits = NULL;

void cleanup(int sock) {
    if (sslCtx != NULL) {
        SSL_CTX_free(sslCtx);
        sslCtx = NULL;
    }

    if (ssl != NULL) {
        SSL_free(ssl);
        ssl = NULL;
    }

    if (hits != NULL) {
        freeaddrinfo(hits);
        hits = NULL;
    }

    close(sock);
}

int httpRequestB(int *sock, short secure, const char *method, const char *host, const char *path, const char *msg, char *buffer, size_t bufSize) {
    int result;
    ssize_t sentBytes;

    if (*sock != 0) {
        struct addrinfo hints, *addr = NULL;

        memset(&hints, 0, sizeof(hints));
        
        hints.ai_family = AF_UNSPEC; 
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        printf("%s\n", host);

        result = getaddrinfo(host, ((secure == 1) ? HTTPS_PORT : HTTP_PORT), &hints, &hits);

        if (result != 0) {
            printf("Here\n");
            return -1;
        }

        addr = hits;
        while (addr != NULL) {
            *sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

            if (*sock == -1) {
                continue;
            }
            
            if (connect(*sock, addr->ai_addr, addr->ai_addrlen) == 0) {
                break;
            }
        }

        if (*sock == -1) {
            freeaddrinfo(hits);
            return -1;
        }

        if (secure == 1 && sslCtx != NULL) {
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_ssl_algorithms();

            sslCtx = SSL_CTX_new(SSLv23_client_method());

            ssl = SSL_new(sslCtx);

            result = SSL_set_fd(ssl, *sock);
            if (result != 0) {
                cleanup(*sock);
                return -1;
            }

            result = SSL_connect(ssl);

            if (result != 0) {
                cleanup(*sock); 
                return -1;
            }
        }
        
        freeaddrinfo(hits);
        hits = NULL;

        if (secure == 1) {
            sentBytes = SSL_write(ssl, msg, strlen(msg));
        } else {
            sentBytes = send(*sock, msg, strlen(msg), 0);
        }

        if (sentBytes == -1) {
            cleanup(*sock);
            return -1;
        }
    }

    if (secure == 1) {
        return SSL_read(ssl, buffer, bufSize);
    } else {
        return read(*sock, buffer, bufSize);
    }
}