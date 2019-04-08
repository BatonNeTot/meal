//
// Created by KILLdon on 30.03.2019.
//

#ifndef MEAL_BASKET_ITER_IMPL_H
#define MEAL_BASKET_ITER_IMPL_H

#include "meal/iter.h"

#include "meal/def.h"

typedef struct iter_t iter_t;

typedef struct const_iter_t const_iter_t;

typedef void (*iter_next_f)(iter_t *iter);

typedef void (*iter_prev_f)(iter_t *iter);

typedef void *(*iter_value_f)(iter_t *iter);

typedef void (*iter_term_f)(iter_t *iter);

typedef iter_t *(*iter_copy_f)(iter_t *iter);

typedef void (*const_iter_next_f)(const_iter_t *iter);

typedef void (*const_iter_prev_f)(const_iter_t *iter);

typedef const void *(*const_iter_value_f)(const_iter_t *iter);

typedef void (*const_iter_term_f)(const_iter_t *iter);

typedef const_iter_t *(*const_iter_copy_f)(const_iter_t *iter);

typedef struct iter_funcs_t {
    iter_next_f next;
    iter_prev_f prev;
    iter_value_f value;
    iter_copy_f copy;
    iter_term_f term;
} iter_funcs_t;

typedef struct const_iter_funcs_t {
    const_iter_next_f next;
    const_iter_prev_f prev;
    const_iter_value_f value;
    const_iter_copy_f copy;
    const_iter_term_f term;
} const_iter_funcs_t;

typedef struct iter_t {
    const iter_funcs_t *funcs;
    void_t data;
} iter_t;

typedef struct const_iter_t {
    const const_iter_funcs_t *funcs;
    const void_t data;
} const_iter_t;


#endif // MEAL_BASKET_ITER_IMPL_H
