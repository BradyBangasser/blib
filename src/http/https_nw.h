#ifndef BLIB_HTTPS_NOT_WINDOWS
#define BLIB_HTTPS_NOT_WINDOWS

// ------WARNING------
// This will not work on windows, see the C++ implentation, https.hpp

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    Socket, secure, http method, host, path, message, buffer, buffer length
    This method makes an HTTPS request to the host and writes the result to the buffer

    returns how many bits were written
*/
int httpRequestB(int *, short, const char *, const char *, const char *, const char *, char *, size_t);


/*

*/

#ifdef __cplusplus 
}
#endif

#endif