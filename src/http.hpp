// This file supports all operating systems
#ifndef BLIB_HTTP
#define BLIB_HTTP

#include "blib_http.h"
#include <string>
#include <vector>

struct RequestOptions {
    // Not impl yet
    bool printProgress = false;
    std::vector<std::string> *headers = NULL;
    const char *content = NULL;
    const char *method = "GET";
    bool secure = true;
};

/*
    url
*/
Blib_Response *request(const std::string, struct RequestOptions * = NULL);

#endif