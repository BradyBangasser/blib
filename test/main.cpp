#include <stdio.h>
#include <string>

// For testing
// -----
#include <winsock2.h>
#include <openssl/ssl.h>

// -----

#include "../src/https_nw.h"
#include "../src/blib_http.h"
#include "../src/http.hpp"

int main() {
    struct RequestOptions opts;
    printf("%llx\n", (long long unsigned int) request("https://www.google.com", &opts));
    return 0;
}