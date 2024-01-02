#ifndef BLIB_HTTPS_NOT_WINDOWS
#define BLIB_HTTPS_NOT_WINDOWS

#include "blib_http.h"

/*
    Method, Host, Path, Buffer, buffer size
*/
int httpsRequestBuf(Blib_Request_Methods,  char *, const char *, char *, size_t);


/*

*/

#endif