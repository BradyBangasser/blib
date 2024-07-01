#pragma once

#include <string.h>
#include "../str/rstrchr.h"

/**
 * @brief 
 * 
 * @param fname The file name
 * @return malloced string
 * @note FREE THE RETURNED STRING
 */
static inline char *rmext(const char *fname) {
    return strndup(fname, (long long int)rstrchr(fname, '.') - (long long int)fname);
}