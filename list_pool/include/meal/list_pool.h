#ifndef MEAL_LIST_POOL_H
#define MEAL_LIST_POOL_H

#include "meal/alloc.h"

#include <stdint.h>
#include <stdbool.h>

typedef struct list_pool_t list_pool_t;

list_pool_t *list_pool_init_via(const alloc_t *alloc, uint32_t typeSize, uint32_t bufferSize);

#define list_pool_init(typeSize, bufferSize) list_pool_init_via(NULL, typeSize, bufferSize)

void list_pool_term(list_pool_t *pool);

void *list_pool_get(list_pool_t *pool);

bool list_pool_has(list_pool_t *pool, void *ptr);

void list_pool_free(list_pool_t *pool, void *ptr);

#endif // MEAL_LIST_POOL_H
