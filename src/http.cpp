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

    char *httpMsg = NULL;
    createHttpMsg(&httpMsg, opts->method, urlInfo->path, urlInfo->host, headers, 10, "");
    printf("%s\n", httpMsg);
    freeHttpMsg(httpMsg);

    freeUrlInfo(urlInfo);
    cleanupWindows();

    return NULL;
}