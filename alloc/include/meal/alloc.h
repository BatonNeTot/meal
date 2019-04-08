#ifndef MEAL_ALLOC_H
#define MEAL_ALLOC_H

#include <stdint.h>

typedef void *(*alloc_malloc_f)(uint32_t size, void *data);
typedef void *(*alloc_realloc_f)(void *ptr, uint32_t size, void *data);
typedef void (*alloc_free_f)(void *ptr, void *data);

typedef struct alloc_funcs_t {
    alloc_malloc_f malloc;
    alloc_realloc_f realloc;
    alloc_free_f free;
} alloc_funcs_t;

typedef struct alloc_t alloc_t;

alloc_t *alloc_init_via(const alloc_t *alloc, const alloc_funcs_t *funcs, void *data);

#define alloc_init(funcs, data) alloc_init_via(NULLm funcs, data)

void alloc_term(alloc_t *alloc);

void *alloc_malloc(const alloc_t *alloc, uint32_t size);

void *alloc_realloc(const alloc_t *alloc, void *ptr, uint32_t size);

void alloc_free(const alloc_t *alloc, void *ptr);

#endif // MEAL_ALLOC_H