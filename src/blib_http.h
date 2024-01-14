#ifndef BLIB_HTTP_LIB
#define BLIB_HTTP_LIB

#include <stdlib.h>

#ifndef BLIB_HTTP_VERSION
#define BLIB_HTTP_VERSION "1.1"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GET,
    POST,
} Blib_Request_Methods;

struct Header {
    unsigned short int status;
    const char *host;
    const char *path;
    const char *rawRes;
    const char *rawHeader;
    const char *mime;
    const char **parsedHeaders;

    size_t headerLen;
    size_t contentLen;
    size_t numberOfHeader;
};

struct Blib_Response {
    /*
        The message sent to the server by the request
    */
    const char *httpMessage;
    struct Header* header;

};

/*
    buf, HTTP method, path, host, headers, number of headers, msg
    Creates an HTTP message to send to a server, it takes care of memory allocing
    call freeHttpMsg when done
    returns the http message str
*/
void createHttpMsg(char **, const char *, const char *, const char *, const char **, size_t, const char *);

/*
    buf
    Frees the memory allocated by createHttpMsg
*/
void freeHttpMsg(char **);

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

#ifdef __cplusplus
}
#endif

#endif