#include "blib_http.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void createHttpMsg(char **buf, const char *method, const char *path, const char *host, const char **headers, size_t headersLen, const char *msg) {
    // This will dynamically allocate memory for the http message
    char *baseString = "%s %s HTTP/%s\r\nHost: %s\r\n\r\n";
    size_t msgLen = snprintf(NULL, 0, baseString, method, path, BLIB_HTTP_VERSION, host);
    char *httpMsg = (char *) malloc(msgLen * sizeof(char));
    snprintf(httpMsg, msgLen, baseString, method, path, BLIB_HTTP_VERSION, host);
    *buf = httpMsg;
}

void freeHttpMsg(char **msg) {
    if (msg != NULL) {
        free(*msg);
        msg = NULL;
    }
}