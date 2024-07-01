#pragma once

#include <string.h>

// String ends with (strew)
static inline int strew(const char *string, const char *end) {
    inline char *startOfEnd = (char *) ((long long int) string + strlen(string) - strlen(end));
    return strcmp(startOfEnd, end);
}