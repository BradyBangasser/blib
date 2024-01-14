#include <stdio.h>
#include <string>

#include "../src/https_nw.h"
#include "../src/blib_http.h"

int main() {
    std::string msg = "sdgafg";
    int i;
    char res[4048] = { 0 };

    char *httpMsg = "";
    createHttpMsg(&httpMsg, "GET", "/", "google.com", NULL, 0, "");
    printf("%s\n", httpMsg);
    printf("here\n");
    freeHttpMsg(&httpMsg);
    // int result = httpRequestB(&i, 1, "GET", "google.com", "", msg.c_str(), res, 4048);
    // printf("%d, %s\n", result, res);
    return 0;
}