#pragma once

#include <sys/stat.h>
#include "str/rstrcmp.h"

/**
 * Recursively create directory
 * Unix Only because Windows is weird
 */
static int rmkdir(const char *path, mode_t mode) {
  char *cpath, *lastSlash;
  int res;

  if (path[0] == 0) return -1;

  if ((res = mkdir(path, mode)) == 0) {
    return 0;
  }

  if (errno == 2) {
    // Either permissions are blocking the creation or it has to create the parent dirtectories first

    if ((lastSlash = rstrchr(path, '/')) == NULL) {
      return -1;
    }

    cpath = strndup(path, lastSlash - path - 1);
    res = rmkdir(cpath, mode);
    free(cpath);

    if (res == -1) {
      return -1;
    }

    return mkdir(path, mode);
  }
}
