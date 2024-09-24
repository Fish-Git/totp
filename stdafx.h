///////////////////////////////////////////////////////////////////////
// stdafx.h: include file for standard system include files,
// or project-specific include files, that are used fairly
// frequently, but which are changed relatively infrequently.
///////////////////////////////////////////////////////////////////////

#pragma once

#define _CRT_SECURE_NO_WARNINGS     // (allow strerror w/o complaint)
#include <windows.h>                // (need SetConsoleMode)

#include <openssl/evp.h>            // OpenSSL EVP_MD struct
#include <openssl/hmac.h>           // OpenSSL HMAC function

typedef __int64 ssize_t;            // (missing from Windows)

//efine strtoul      strtoul        // (already exists)
#define strtoull    _strtoui64      // (Windows's name)
#define strtoll     _strtoi64       // (Windows's name)
#define strncasecmp _strnicmp       // (Windows's name)
#define time        _time64         // (want 64-bit time)

// Add any additional #include headers or #define constants here...

#include "Version.h"	            // our version #defines
