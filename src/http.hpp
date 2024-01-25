// This file supports all operating systems
#ifndef BLIB_HTTP
#define BLIB_HTTP

#include "blib_http.h"
#include <string>
#include <vector>

#define BLIB_HEADER(n, v) { n, v, (strlen(n) + strlen(v)) }

namespace blib_http {
    namespace error_codes {
        inline constexpr int FAILURE_TO_INIT_SOCKET = -10;
        inline constexpr int FAILURE_TO_INIT_SSL = -11;
        inline constexpr int FAILURE_TO_CONNECT = -20;
        inline constexpr int SSL_FAILURE_TO_CONNECT = -21;
        inline constexpr int FAILURE_TO_OPEN_OUT_FILE = -30;
        inline constexpr int SSL_FAILURE_TO_WRITE_DATA = -40;
        inline constexpr int FAILURE_TO_WRITE_DATA = -41;
        inline constexpr int SSL_FAILURE_TO_READ = -50;
        inline constexpr int FAILURE_TO_READ = -51;
        inline constexpr int FAILURE_TO_ALLOC_MEM = -100;
    }

    struct RequestOptions {
        // Not impl yet
        bool printProgress = false;
        std::vector<Blib_Header> *headers = NULL;
        const char *content = NULL;
        const char *method = "GET";
        // remove this, for testing only
        bool secure = true;
    };

    /*
        url
    */
    int request(const std::string, const std::string, struct RequestOptions * = NULL);
    const char *request(const std::string, struct RequestOptions * = NULL);

}

#endif