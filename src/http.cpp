#include "http.hpp"
#include "blib_http.h"

#include <string>
#include <stdio.h>
#include <string.h>

#define BLIB_HEADER(n, v) { n, v, (strlen(n) + strlen(v)) }

struct Blib_Response *request(std::string url, struct RequestOptions *opts) {
    if (opts == NULL) {
        struct RequestOptions options;
        opts = &options;
    }

    initWindows();
    struct UrlInfo *urlInfo = getUrlInfo(url.c_str());

    struct Blib_Header headers[2] = {
        BLIB_HEADER("coc", "ball"),
        BLIB_HEADER("thy", "cars"),
    };

    char *httpMsg = NULL;
    createHttpMsg(&httpMsg, opts->method, urlInfo->path, urlInfo->host, headers, 2, "");
    printf("%s\n", httpMsg);
    freeHttpMsg(httpMsg);

    freeUrlInfo(urlInfo);
    cleanupWindows();

    return NULL;
}