#ifndef clox_common_h
#define clox_common_h

#include "settings.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ERROR_GUARD(expr) \
    do {\
        if ((expr) == -1) {\
            return -1;\
        }\
    } while (false)

#ifdef _WIN32
#define PLATFORM_WINDOWS
#endif

#if defined(unix) || defined(__unix__) || defined(__unix) || \
    (defined(__APPLE__) && defined(__MACH__))
# define PLATFORM_UNIX
#endif

// Attempt at platform-independent print format modifiers
#ifdef PLATFORM_WINDOWS
#define FORMAT_SIZE_T "Iu"
#else
#define FORMAT_SIZE_T "zu"
#endif

#endif