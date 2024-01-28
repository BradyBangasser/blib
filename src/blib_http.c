#define WIN32_LEAN_AND_MEAN

#include "blib_http.h"
#include "blib_constants.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifndef _WIN32
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <ws2tcpip.h>
#include <winsock2.h>
#endif

#include <openssl/types.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

// Make this dynamically allocated and freed somehow
static char errmsg[128] = { 0 };
static int err = 0;

int getErrorCode() {
    return err;
}

const char *getErrorMsg() {
    return errmsg;
}

int initSock(struct addrinfo *addr) {
    BXINFO("Attempting to init socket\n");
    int sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (sock == -1) {
        err = errno;
        BXINFO("Error creating sock: %i\n", err);
        return -1;
    }

    BXINFO("Success\n");
    return sock;
}

void cleanupSock(int sock) {
    int result;
    // Shut down both in and out from the socket
    result = shutdown(sock, 0x02);
    // Finish this warning
    if (result )
    #ifndef _WIN32
    close(sock);
    #else
    closesocket(sock);
    #endif
}

int initSSL(int sock, SSL **ssl, SSL_CTX **ctx) {
    BXINFO("Attempting to init SSL\n");
    SSL_library_init();
    OpenSSL_add_ssl_algorithms();

    BXINFO("Attempting to create SSL context\n");
    *ctx = SSL_CTX_new(SSLv23_client_method());

    if (*ctx == NULL) {
        err = ERR_get_error();
        BINFO("Error creating SSL ctx: %i\n", err);
        ssl = NULL;
        return -1;
    }
    BXINFO("Created SSL Content\n");

    BXINFO("Attempting to create SSL structure\n");
    *ssl = SSL_new(*ctx);

    if (*ssl == NULL) {
        err = ERR_get_error();
        BINFO("Error creating SSL structure: %i\n", err);
        // This function will set ctx to null
        cleanupSSL(ssl, ctx);
        return -2;
    }

    BXINFO("Successfully created SSL structure");
    BXINFO("Attempting to set SSL file descriptor\n");
    int result = SSL_set_fd(*ssl, sock);

    if (result != 1) {
        err = SSL_get_error(ssl, result);
        BXINFO("Error setting SSL file descriptor: %i\n", err);
        cleanupSSL(ssl, ctx);
        return -3;
    }

    BXINFO("Success\n");
    BXINFO("Successfully init SLL\n");
    return 0;
}

void cleanupSSL(SSL **ssl, SSL_CTX **ctx) {
    BXINFO("Attempting to cleanup SSL\n");
    if (*ctx != NULL) {
        SSL_CTX_free(*ctx);
        *ctx = NULL;
    }

    if (*ssl != NULL) {
        SSL_free(*ssl);
        *ssl = NULL;
    }

    BXINFO("Freed\n");
}

int createHttpMsg(char **buf, const char *method, const char *path, const char *host, struct Blib_Header *headers, size_t headersLen, const char *msg) {
    BXINFO("Starting HttpMsg generation\n");
    // This will dynamically allocate memory for the http message
    int i;
    char *curs = NULL, *headersString = calloc(1, sizeof(char));
    const char *currentHeader;

    if (headersString == NULL) {
        BXINFO("Error allocating headersString\n");
        *buf = NULL;
        return -1;
    }

    if (headersLen > 0 && headers != NULL) {
        BXINFO("Starting Header Parsing\n");
        BXINFO("Attempting to create first header\n");
        currentHeader = createHeaderString(headers);

        if (currentHeader == NULL) {
            BXINFO("Error creating first header\n");
            free(headersString);
            *buf = NULL;
            return -1;
        }

        // Allocs the size of the first header, plus the null character
        headersString = realloc(headersString, (1 + strlen(currentHeader)) * sizeof(char));

        if (headersString == NULL) {
            // cry
            BXINFO("Error reallocing headersString\n");
            *buf = NULL;
            return -1;
        }
        
        // sprints the headers, and the null character to the string
        snprintf(headersString, strlen(currentHeader) + 1, currentHeader);

        freeHeaderString(&currentHeader);

        if (headersLen > 1) {
            BXINFO("Attempting to create remaining headers\n");
            for (i = 1; i < headersLen; i++) {
                BXINFO("Creating header string no %i\n", i);
                currentHeader = createHeaderString(headers + i);

                if (currentHeader == NULL) {
                    BXINFO("Error creating header string no %i\n", i);
                    free(headersString);
                    *buf = NULL;
                    return -1;
                }
                // allocs the strlen of the entire header string up to this point plus the size of the current header and a null character
                BXINFO("Increasing headersString size\n");
                headersString = (char *) realloc(headersString, (strlen(headersString) + strlen(currentHeader) + 1) * sizeof(char));

                // Starts writing at the previous headers null character
                if (headersString != NULL) curs = headersString + strlen(headersString);
                else {
                    BXINFO("Error increasing headersString size\n");
                    freeHeaderString(currentHeader);
                    // cry
                    *buf = NULL;
                    return -1;
                }

                BXINFO("Sucess\n");

                snprintf(curs, strlen(currentHeader) + 1, currentHeader);
                freeHeaderString(&currentHeader);

                BXINFO("Created header string no %i\n", i);
            }

            BXINFO("Created all header string\n");
        }
    }

    BXINFO("Attempting to assemble HTTP Msg\n");

    char *baseString = "%s %s HTTP/%s\r\n%sHost: %s\r\nContent-Length: %lli\r\n\r\n%s\r\n";
    size_t msgLen = snprintf(NULL, 0, baseString, method, path, BLIB_HTTP_VERSION, headersString, host, strlen(msg), msg) + 2;
    char *httpMsg = (char *) malloc(msgLen * sizeof(char));

    if (httpMsg == NULL) {
        free(headersString);
        BXINFO("Error mallocing httpMsg\n");
        *buf = NULL;
        return -1;
    }

    snprintf(httpMsg, msgLen, baseString, method, path, BLIB_HTTP_VERSION, headersString, host, strlen(msg), msg);
    free(headersString);
    *buf = httpMsg;
    BXINFO("Sucess\n");
}

void freeHttpMsg(char **msg) {
    BXINFO("Freeing HTTP Msg\n");
    if (*msg != NULL) {
        free(*msg);
        *msg = NULL;
    }
}

struct addrinfo *blibGetAddrInfo(const char *host, const char *port) {
    BXINFO("Getting address info for %s:%s\n", host, port);
    struct addrinfo hints, *addr;

    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    memset(&hints, 0, sizeof hints);

    int result = getaddrinfo(host, port, &hints, &addr);

    if (result != 0) {
        err = errno;
        BXINFO("Error getting addrinfo: %i\n", err);
        return NULL;
    } else {
        BXINFO("Success, found addresses for %s:%s\n", host, port);
        return addr;
    }
}

void blibFreeAddrInfo(struct addrinfo **addrinfo) {
    BXINFO("Freeing address info\n");
    if (*addrinfo != NULL) {
        freeaddrinfo(*addrinfo);
        *addrinfo = NULL;
    }
}

struct UrlInfo *getUrlInfo(const char *url) {
    BXINFO("Parsing url '%s'\n", url);
    size_t len;
    char *tok, *str;

    BXINFO("Attempting to malloc urlInfo struct\n");
    struct UrlInfo *urlInfo = malloc(sizeof(struct UrlInfo));
    if (urlInfo == NULL) return NULL;
    BXINFO("Success\n");

    BXINFO("Attempting to malloc url\n");
    len = strlen(url) + 1;
    urlInfo->url = (char *) malloc(sizeof(char) * len);

    if (urlInfo->url == NULL) {
        BXINFO("Failed allocate mem\n");
        freeUrlInfo(&urlInfo);
        return NULL;
    }

    snprintf(urlInfo->url, len, url);

    BXINFO("Success\n");
    BXINFO("Finding proto\n");

    tok = strstr(urlInfo->url, "://");
    if (tok == NULL) {
        freeUrlInfo(urlInfo);
        BXINFO("Invalid URL\n");
        return NULL;
    }

    len = tok - urlInfo->url + 1;
    urlInfo->proto = (char *) malloc(sizeof(char) * len);
    if (urlInfo->proto == NULL) {
        BXINFO("Failed to allocate mem for proto\n");
        freeUrlInfo(&urlInfo);
        return NULL;
    }
    snprintf(urlInfo->proto, len, urlInfo->url);
    BXINFO("Success, Proto: %s\n", urlInfo->proto);

    BXINFO("Attempting to get host\n");
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
        BXINFO("Failed to allocate mem for host\n");
        freeUrlInfo(&urlInfo);
        return NULL;
    }

    snprintf(urlInfo->host, len, str);

    BXINFO("Success, Host: %s\n", urlInfo->host);

    BXINFO("Attempting to get path\n");

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
        BXINFO("Failed to allocate mem for path\n");
        freeUrlInfo(&urlInfo);
        return NULL;
    }

    snprintf(urlInfo->path, len, tok);
    BXINFO("Success, Path: %s\n", urlInfo->path);

    BXINFO("Getting port no\n");
    char *protoPort = (strcmp(urlInfo->proto, "http") == 0 ? HTTP_PORT : HTTPS_PORT);

    urlInfo->addrInfo = blibGetAddrInfo(urlInfo->host, protoPort);
    if (urlInfo->addrInfo == NULL) {
        BXINFO("Failed to allocate mem for port\n");
        freeUrlInfo(&urlInfo);
        return NULL;
    }

    BXINFO("Success, port: %s\n", protoPort);

    return urlInfo;
}

void freeUrlInfo(struct UrlInfo **urlInfo) {
    BXINFO("Freeing Url Info\n");
    if (*urlInfo != NULL) {
        if ((*urlInfo)->proto != NULL) free((char *) (*urlInfo)->proto);
        if ((*urlInfo)->host != NULL) free((char *) (*urlInfo)->host);
        if ((*urlInfo)->path != NULL) free((char *) (*urlInfo)->path);
        if ((*urlInfo)->url != NULL) free((char *) (*urlInfo)->url);
        if ((*urlInfo)->addrInfo != NULL) blibFreeAddrInfo(&(*urlInfo)->addrInfo);
        free(*urlInfo);
        *urlInfo = NULL;
    }
}

const char *createHeaderString(struct Blib_Header *header) {
    BXINFO("Creating formatted header string\n");
    const char *baseString = "%s: %s\r\n";
    size_t headerStringLen = snprintf(NULL, 0, baseString, header->name, header->value) + 2;
    char *headerString = (char *) malloc(headerStringLen * sizeof(char));

    if (headerString == NULL) {
        BXINFO("Error mallocing headerString\n");
        return NULL;
    }

    snprintf(headerString, headerStringLen, baseString, header->name, header->value);
    BXINFO("Successfully created header string: %s\n", headerString);

    return headerString;
}

void freeHeaderString(const char **headerString) {
    BXINFO("Freeing header string: %s\n", *headerString);
    if (*headerString != NULL) {
        free((void *) *headerString);
        *headerString = NULL;
    }
}

struct Blib_Response_Header *parseResponseHeader(const char *rawHeader) {
    BXINFO("Attemtping to parse header: %s\n", rawHeader);
    // Personal challenge for later, rewrite to only use 1 malloc call
    char *curs, *m_header, *currentHeader;
    char *raw, *statusMsg;
    unsigned short status;
    size_t headerLen, numberOfHeaders = 0, lineLen;

    // Get a pointer for the headers
    struct Blib_Header *headers = malloc(0);

    if (headers == NULL) {
        BXINFO("Error allocating mem for headers array\n");
        return NULL;
    }

    curs = strstr(rawHeader, "\r\n\r\n");
    if (curs == NULL) {
        BXINFO("Header is invalid\n");
        free(headers);
        return NULL;
    }

    headerLen = curs - rawHeader;

    raw = malloc(headerLen * sizeof(char));

    if (raw == NULL) {
        free(headers);
        BXINFO("Failed to allocate mem for header copy\n");
        return NULL;
    }

    m_header = malloc(headerLen * sizeof(char));

    if (m_header == NULL) {
        free(raw); 
        free(headers);
        BXINFO("Failed to allocate mem for header copy\n");
        return NULL;
    }

    BXINFO("Attempting to parse first line of header\n");
    strncpy(raw, rawHeader, headerLen);
    strncpy(m_header, rawHeader, headerLen);

    curs = strtok(m_header, "\r\n");

    if (curs == NULL) {
        free(raw);
        free(m_header);
        free(headers);
        BXINFO("Invalid Header Syntax (Unable to parse first line)\n");
        return NULL;
    }

    // process first line
    // Expect format HTTP/x.x status (3 digits) msg
    curs = strstr(curs, " ");

    if (curs == NULL) {
        free(raw);
        free(m_header);
        free(headers);
        BXINFO("Invalid Header Syntax (Unable to parse first line)\n");
        return NULL;
    }

    // Remove Space
    curs++;
    char statusString[4];
    memcpy(statusString, curs, 3);
    statusString[4] = '\0';
    status = atoi(statusString);

    curs = strstr(curs, " ");
    // Remove space
    curs++;
    lineLen = strlen(curs);

    statusMsg = malloc(lineLen + 1);

    if (statusMsg == NULL) {
        BXINFO("Unable to allocate mem for status msg\n");
        free(raw);
        free(m_header);
        free(headers);
        return NULL;
    }

    strncpy(statusMsg, curs, lineLen);
    statusMsg[strlen(curs)] = '\0';

    BXINFO("Starting header parsing\n");

    // It may be worth adding another loop and only calling malloc once for all of the structs
    // It also may be worth it to get rid of the structs and just use strings and realloc calls to save memory or speed
    while (currentHeader = strtok(NULL, "\r\n"), currentHeader != NULL && strlen(currentHeader) > 0) {
        char *headerString = malloc(strlen(currentHeader) + 1 * sizeof(char));

        if (headerString == NULL) {
            free(raw);
            free(statusMsg);
            free(m_header);
            free(headers);
            BXINFO("Unable to allocate mem for status msg\n");
            return NULL;
        }

        headerString[strlen(currentHeader)] = '\0';
        memcpy(headerString, currentHeader, strlen(currentHeader));

        curs = strstr(headerString, ": ");

        if (curs == NULL) {
            free(raw);
            free(headerString);
            free(statusMsg);
            free(headers);
            free(m_header);
            BXINFO("Invalid Header Syntax (Unable to parse first line)\n");
            return NULL;
        }

        *curs = '\0';
        memcpy(curs + 1, curs + 2, strlen(curs + 1));


        numberOfHeaders++;
        headers = realloc(headers, numberOfHeaders * sizeof(struct Blib_Header));
        if (headers == NULL) {
            free(raw);
            free(headerString);
            free(statusMsg);
            free(headers);
            free(m_header);
            BXINFO("Invalid Header Syntax (Unable to parse first line)\n");
            return NULL;
        }
        headers[numberOfHeaders - 1] = (struct Blib_Header){
            headerString,
            curs + 1
        };

        BXINFO("Parsed header: %s\n", headerString);
    }

    struct Blib_Response_Header res = {
        status,
        statusMsg,
        raw,
        headers,
        numberOfHeaders,
        strlen(raw),
    };

    struct Blib_Response_Header *resPtr = malloc(sizeof(struct Blib_Response_Header));
    memcpy(resPtr, &res, sizeof(res));

    free(m_header);

    return resPtr;
}

void freeResponseHeader(struct Blib_Response_Header **header) {
    struct Blib_Response_Header *h = (struct Blib_Response_Header *) *header;
    if (h != NULL) {
        free((void *) h->raw);
        free((void *) h->statusMsg);
        struct Blib_Header *phs = (struct Blib_Header *) h->parsedHeaders;

        for (int i = 0; i < h->numberOfHeader; i++) {
            free((void *) (phs + i)->name);
        }

        free((void *) h->parsedHeaders);

        free((void *) h);
        *header = NULL;
    }
}

const char *getHeaderValue(const struct Blib_Response_Header *headers, const char *headerName) {
    for (int i = 0; i < headers->numberOfHeader; i++) {
        if (strcmp(headers->parsedHeaders[i].name, headerName) == 0) {
            return headers->parsedHeaders[i].value;
        }
    }

    return NULL;
}