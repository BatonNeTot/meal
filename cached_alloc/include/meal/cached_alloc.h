#ifndef MEAL_CACHED_ALLOC_H
#define MEAL_CACHED_ALLOC_H

#include "meal/alloc.h"

#include <stdint.h>

typedef struct cached_alloc_t cached_alloc_t;

cached_alloc_t *cached_alloc_init_via(const alloc_t *alloc, uint32_t bufferSize);

#define cached_alloc_init(bufferSize) cached_alloc_init_via(NULL, bufferSize)

void cached_alloc_term(cached_alloc_t *alloc);

const alloc_t *cached_alloc_as_alloc(cached_alloc_t *alloc);

void *cached_alloc_malloc(cached_alloc_t *_alloc, uint32_t size);

void *cached_alloc_realloc(cached_alloc_t *alloc, void *ptr, uint32_t size);

void cached_alloc_free(cached_alloc_t *alloc, void *ptr);

#endif // MEAL_CACHED_ALLOC_H