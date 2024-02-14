#include <stdio.h>
#include <string>

// For testing
// -----
#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#endif

#include <openssl/ssl.h>

// -----

#include "../src/http/https_nw.h"
#include "../src/http/blib_http.h"
#include "../src/http/http.hpp"

using namespace blib_http;

int main() {
    const std::string url = "https://www.google.com";
    std::string result = request<const std::string>(url);
    int result1 = request<int>(url, "out.html");
    // printf("%s this\n", result.c_str());
    
    return 0;
}