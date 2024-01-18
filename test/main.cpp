#include <stdio.h>
#include <string>

#include "../src/https_nw.h"
#include "../src/blib_http.h"
#include "../src/http.hpp"

int main() {
    std::string url = "https://www.google.com?this";
    
    struct RequestOptions opts;
    opts.secure = false;
    

    printf("%x\n", request(url, &opts));
    return 0;
}