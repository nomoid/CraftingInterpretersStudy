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

#endif