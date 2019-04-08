#ifndef MEAL_BASKET_RB_TREE_H
#define MEAL_BASKET_RB_TREE_H

#include "meal/def.h"
#include "meal/alloc.h"
#include "meal/iter.h"

#include <stdbool.h>

typedef struct rb_tree_t rb_tree_t;

rb_tree_t *rb_tree_init_via(const alloc_t *alloc, cmp_f compare, uint32_t typeSize, uint32_t bufferSize);

#define rb_tree_init(compare, typeSize, bufferSize) rb_tree_init_via(NULL, compare, typeSize, bufferSize)

void rb_tree_term(rb_tree_t *tree);

iter_t *rb_tree_iter_tmp(rb_tree_t *tree, const void *ptr);

void *rb_tree_insert(rb_tree_t *tree, const void *ptr);

iter_t *rb_tree_insert_iter(rb_tree_t *tree, const void *ptr);

void *rb_tree_find(rb_tree_t *tree, const void *ptr);

void *rb_tree_min(rb_tree_t *tree, const void *ptr);

void *rb_tree_max(rb_tree_t *tree, const void *ptr);

void rb_tree_foreach(rb_tree_t *tree, action_f func, void *data);

iter_t *rb_tree_iter(rb_tree_t *tree);

iter_t *rb_tree_find_iter(rb_tree_t *tree, const void *ptr);

iter_t *rb_tree_min_iter(rb_tree_t *tree, const void *ptr);

iter_t *rb_tree_max_iter(rb_tree_t *tree, const void *ptr);

bool rb_tree_remove(rb_tree_t *tree, const void *ptr, void *dst);

bool rb_tree_remove_min(rb_tree_t *tree, const void *ptr, void *dst);

bool rb_tree_remove_max(rb_tree_t *tree, const void *ptr, void *dst);

bool rb_tree_remove_iter(rb_tree_t *tree, iter_t *iter, void *dst);

uint32_t rb_tree_size(rb_tree_t *tree);

void rb_tree_clear(rb_tree_t *tree);

#endif //MEAL_BASKET_RB_TREE_H