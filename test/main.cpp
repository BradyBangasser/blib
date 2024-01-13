#include <stdio.h>

#include "../src/https_nw.h"

int main() {
    int i;
    char res[4048] = { 0 };
    int result = httpRequestB(&i, 1, "GET", "google.com", "", "", res, 4048);
    printf("%d, %s\n", result, res);
    return 0;
}