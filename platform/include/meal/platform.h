#ifndef MEAL_PLATFORM_H
#define MEAL_PLATFORM_H

#define _x32

//#define _x64

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