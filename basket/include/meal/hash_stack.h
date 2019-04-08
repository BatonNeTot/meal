#ifndef MEAL_BASKET_HASH_STACK_H
#define MEAL_BASKET_HASH_STACK_H

#include "meal/alloc.h"

#include <stdint.h>

typedef struct hash_stack_t hash_stack_t;

hash_stack_t *hash_stack_init_via(const alloc_t *alloc, uint32_t typeSize, uint32_t bufferSize);

#define hash_stack_init(typeSize, bufferSize) hash_stack_init_via(NULL, typeSize, bufferSize)

void hash_stack_term(hash_stack_t *stack);

void *hash_stack_push(hash_stack_t *stack, const void *ptr);

void hash_stack_pop_count(hash_stack_t *stack, uint32_t count);

#define hash_stack_pop(stack) hash_stack_pop_count(stack, 1)

void *hash_stack_peek_offset(hash_stack_t *stack, uint32_t index);

#define hash_stack_peek(stack) hash_stack_peek_offset(stack, 1)

uint32_t hash_stack_size(hash_stack_t *stack);

uint32_t hash_stack_type_size(hash_stack_t *stack);

uint32_t hash_stack_buffer_size(hash_stack_t *stack);

void hash_stack_clear(hash_stack_t *stack);

void hash_stack_clear_hard(hash_stack_t *stack);

#endif // MEAL_BASKET_HASH_STACK_H