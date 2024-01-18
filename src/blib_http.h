#ifndef BLIB_HTTP_LIB
#define BLIB_HTTP_LIB

#include <stdlib.h>
#include <openssl/ssl.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#include <ws2tcpip.h>
#endif

#ifndef BLIB_HTTP_VERSION
#define BLIB_HTTP_VERSION "1.1"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct Header {
    unsigned short int status;
    char *host;
    char *path;
    char *rawRes;
    char *rawHeader;
    char *mime;
    char **parsedHeaders;

    size_t headerLen;
    size_t contentLen;
    size_t numberOfHeader;
};

struct UrlInfo {
    char *url;
    char *host;
    char *path;
    char *proto;
    struct addrinfo *addrInfo;
};

struct Blib_Header {
    const char *name;
    const char *value;
    size_t totalLen;
};

struct Blib_Response {
    /*
        The message sent to the server by the request
    */
    const char *httpMessage;
    struct Header* header;
    const char *content;
};

/*
    This inits winsock
    Can be used on any OS
*/
inline void initWindows() {
    #ifdef _WIN32
    // Why windows why
    struct WSAData winData;
    WSAStartup(MAKEWORD(2,2), &winData);
    #endif
}

/*
This cleanup winsock
Can be used on any OS
*/
inline void cleanupWindows() {
    #ifdef _WIN32
    WSACleanup();
    #endif
}

/*
    This function converts the Blib_Header struct into a string to put into an http message
    Adds \r\n to the end of the header
    You need to call freeHeaderString when done
    returns a string in the following format: "name: value\r\n"
*/
const char *createHeaderString(struct Blib_Header *);

/*
    Frees the memeory alloced by createHeaderString
*/
void freeHeaderString(const char *);

/*
    buf, HTTP method, path, host, headers, number of headers, msg
    Creates an HTTP message to send to a server, it takes care of memory allocing
    call freeHttpMsg when done
    returns the http message str
*/
void createHttpMsg(char **, const char *, const char *, const char *, struct Blib_Header*, size_t, const char *);

/*
    buf
    Frees the memory allocated by createHttpMsg
*/
void freeHttpMsg(char *);

/*
    address information
    create a network socket
    You must call cleanupSock when done
    return pointer to socket or null if the socket initation fails
*/
int *initSock(struct addrinfo *);

/*
    pointer to sock
    shutsdown and free the socket
*/
void cleanupSock(int *);

/*
    socket, struct SSL, struct SSL context
*/
void initSSL(int *, SSL **, SSL_CTX **);

/*
    ssl, ssl ctx
    frees up the ssl and ssl ctx memory
*/
void cleanupSSL(SSL *, SSL_CTX *);

/*
    Raw response data, header struct pointer
    Parses the header data out of the raw response and modifies the header in place
    You need to call freeHeader when done with the header to avoid memory leaks
    Returns BLIB_OK on success and an error code on failure
*/
int parseHeader(const char *, struct Header *);

/*
    header struct pointer
    Frees the data malloced in parseHeader
*/
void freeHeader(struct Header *);

/*
    host, port
    gets the addrinfo
    supports all operating systems
    returns a pointer to the addrinfo struct
*/
struct addrinfo *blibGetAddrInfo(const char *, const char *);

/*
    addrinfo pointer
    frees the addr info 
*/
void blibFreeAddrInfo(struct addrinfo *);

/*
    url
    This will parse a url
    returns a pointer to a UrlInfo struct
    You must call freeUrlInfo when done with the url info
*/
struct UrlInfo *getUrlInfo(const char *);

/*

*/
void freeUrlInfo(struct UrlInfo *);

#ifdef __cplusplus
}
#endif

#endif