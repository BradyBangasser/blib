#include <stdio.h>
#include <string>

// For testing
// -----
#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#endif

#include <openssl/ssl.h>

// -----

#include "../src/https_nw.h"
#include "../src/blib_http.h"
#include "../src/http.hpp"

using namespace blib_http;

int main() {
    const std::string url = "https://google.com";
    int result = request<int>(url, "st");
    
    return 0;
}