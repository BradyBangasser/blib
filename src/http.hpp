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
    const size_t contentLength = 0;
    const char *method = "GET";
};

/*
    url
*/
Blib_Response *request(const std::string, struct RequestOptions * = NULL);

#endif