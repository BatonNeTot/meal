#ifndef MEAL_DEF_H
#define MEAL_DEF_H

#include <stdint.h>

typedef struct {} void_t;

typedef int32_t (*cmp_f)(const void *ptr1, const void *ptr2);

typedef void (*action_f)(void *ptr, void *data);

typedef void (*const_action_f)(const void *ptr, void *data);

#endif //MEAL_DEF_H