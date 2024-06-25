#pragma once

#include <string.h>

/**
 * strchr but it looks for the last instance of a character
*/

static inline char *rstrchr(const char *str, char c) {
  int i = strlen(str);
  while (i-- > 0) if (str[i] == c) return str + i;
  return NULL;
}
