#ifndef MEAL_BASKET_ITER_H
#define MEAL_BASKET_ITER_H

#include "meal/alloc.h"

#include <stdbool.h>

typedef struct iter_t iter_t;

typedef struct const_iter_t const_iter_t;

iter_t *iter_copy(iter_t *iter);

void iter_term(iter_t *iter);

void iter_next(iter_t *iter);

void iter_prev(iter_t *iter);

void *iter_value(iter_t *iter);

const_iter_t *const_iter_copy(const_iter_t *iter);

void const_iter_term(const_iter_t *iter);

void const_iter_next(const_iter_t *iter);

void const_iter_prev(const_iter_t *iter);

const void *const_iter_value(const_iter_t *iter);

#endif // MEAL_BASKET_ITER_H