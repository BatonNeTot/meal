#ifndef MEAL_PLATFORM_H
#define MEAL_PLATFORM_H

#if !defined(_x32) && !defined(_x64)
    #if defined(__i386) || defined(__i386__) || defined(_M_IX86)
        #define _x32
    #elif defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
        #define _x64
    #else
        #error "Can't define architecture, select manually"
    #endif
#endif

#include <stdint.h>

#if defined(_x32)

#define WSBIT 32
#define WSB 4
#define WSL2 2
#define WST uint32_t

#elif defined(_x64)

#define WSBIT 64
#define WSB 8
#define WSL2 3
#define WST uint64_t

#else

#error No word was selected

#endif

#endif // MEAL_PLATFORM_H