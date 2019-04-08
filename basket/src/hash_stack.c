#include "meal/hash_stack.h"

#include "meal/memory.h"
#include "meal/macros.h"
#include "meal/assert.h"

#define TAG "Hash Stack"

typedef struct hash_stack_t {
    const alloc_t *alloc;
    uint32_t typeSize;
    uint32_t bufferSize;
    uint32_t size;
    uint32_t levels;
    unsigned long long cap;
    void *data;
} hash_stack_t;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
hash_stack_t *hash_stack_init_via(const alloc_t *alloc, uint32_t typeSize, uint32_t bufferSize) {
    ASSERT_ERROR(typeSize, TAG, "TypeSize must be more than 0: typeSize = %d", typeSize) {
        return NULL;
    }

    ASSERT_ERROR(bufferSize > 1, TAG, "BufferSize must be more than 1: bufferSize = %d", bufferSize) {
        return NULL;
    }

    hash_stack_t *stack = alloc_malloc(alloc, sizeof(hash_stack_t));

    ASSERT_ERROR(stack, TAG, "Can't allocate memory for stack") {
        return NULL;
    }

    stack->alloc = alloc;
    stack->typeSize = typeSize;
    stack->bufferSize = bufferSize;
    stack->size = 0;
    stack->levels = 0;

    stack->data = alloc_malloc(alloc, stack->typeSize * stack->bufferSize);

    ASSERT_ERROR(stack->data, TAG, "Can't allocate memory for stack data") {
        alloc_free(alloc, stack);
        return NULL;
    }

    stack->cap = stack->bufferSize;

    return stack;
}
#pragma clang diagnostic pop

static void _hash_stack_clear(const alloc_t *alloc, void *data, uint32_t size, uint32_t level);

#define HASH_STACK_CLEAR(stack) \
if (stack->levels) {\
    _hash_stack_clear(stack->alloc, stack->data, stack->bufferSize, stack->levels);\
} else if (stack->levels > 1) {\
    alloc_free(stack->alloc, stack->data);\
}\

void hash_stack_term(hash_stack_t *stack) {
    ASSERT_ERROR(stack, TAG, "NULL stack") {
        return;
    }

    HASH_STACK_CLEAR(stack);

    alloc_free(stack->alloc, stack);
}

#define HASH_STACK_ADDRESS(stack, address, index)\
uint32_t __INDEX = index;\
uint32_t address = __INDEX % stack->bufferSize;\
LOOP(i, stack->levels) {\
    __INDEX /= stack->bufferSize;\
    address *= stack->bufferSize;\
    address += __INDEX % stack->bufferSize;\
}\

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
void *hash_stack_push(hash_stack_t *stack, const void *ptr) {
    ASSERT_ERROR(stack, TAG, "NULL stack") {
        return NULL;
    }

    ASSERT_ERROR(ptr, TAG, "NULL data") {
        return NULL;
    }

    if (stack->size == stack->cap) {
        void **array = alloc_malloc(stack->alloc, sizeof(void *) * stack->bufferSize);

        ASSERT_ERROR(array, TAG, "Can't allocate memory for stack data") {
            return NULL;
        }

        array[0] = stack->data;
        for (uint32_t i = 1; i < stack->bufferSize; i++) {
            array[i] = NULL;
        }

        stack->data = array;
        stack->levels++;
        stack->cap *= stack->bufferSize;
    }

    void **array = stack->data;
    HASH_STACK_ADDRESS(stack, address, stack->size)

    for (int32_t i = stack->levels - 1; i >= 0; i--) {
        uint32_t mod = address % stack->bufferSize;

        if (!array[mod]) {
            void **newArray;

            if (i) {
                newArray = alloc_malloc(stack->alloc, sizeof(void *) * stack->bufferSize);

                ASSERT_ERROR(newArray, TAG, "Can't allocate memory for stack data") {
                    return NULL;
                }

                for (uint32_t j = 0; j < stack->bufferSize; j++) {
                    newArray[j] = NULL;
                }
            } else {
                newArray = alloc_malloc(stack->alloc, stack->typeSize * stack->bufferSize);

                ASSERT_ERROR(newArray, TAG, "Can't allocate memory for stack data") {
                    return NULL;
                }
            }

            array[mod] = newArray;
        }

        array = array[mod];
        address /= stack->bufferSize;
    }

    void *dst = (void *)array + stack->typeSize * address;
    mem_copy(dst, ptr, stack->typeSize);

    stack->size++;
    return dst;
}
#pragma clang diagnostic pop

#define HASH_STACK_PEEK(stack, address, array) \
void **array = stack->data;\
LOOP(i, stack->levels) {\
    array = (void **)array[address % stack->bufferSize];\
    address /= stack->bufferSize;\
}

void hash_stack_pop_count(hash_stack_t *stack, uint32_t count) {
    ASSERT_ERROR(stack, TAG, "NULL stack") {
        return;
    }

    ASSERT_ERROR(stack->size, TAG, "Stack is empty") {
        return;
    }

    ASSERT_WARNING(count > 0, TAG, "Count to pop is zero");

    ASSERT_ERROR(count <= stack->size, TAG, "Count to pop more than stack size: size = %d, count = %d", stack->size, count) {
        return;
    }

    stack->size -= count;
}

void *hash_stack_peek_offset(hash_stack_t *stack, uint32_t index) {
    ASSERT_ERROR(stack, TAG, "NULL stack") {
        return NULL;
    }

    ASSERT_ERROR(index > 0, TAG, "Index must be more than 0: index = %d", index) {
        return NULL;
    }

    ASSERT_ERROR(stack->size >= index, TAG, "Index must be less than stack size: "
                                                "size = %d; index = %d", stack->size, index) {
        return NULL;
    }

    HASH_STACK_ADDRESS(stack, address, stack->size - index)

    HASH_STACK_PEEK(stack, address, array)
    return (void *)array + address * stack->typeSize;
}

uint32_t hash_stack_size(hash_stack_t *stack) {
    ASSERT_ERROR(stack, TAG, "NULL stack") {
        return 0;
    }

    return stack->size;
}

uint32_t hash_stack_type_size(hash_stack_t *stack) {
    ASSERT_ERROR(stack, TAG, "NULL stack") {
        return 0;
    }

    return stack->typeSize;
}

uint32_t hash_stack_buffer_size(hash_stack_t *stack) {
    ASSERT_ERROR(stack, TAG, "NULL stack") {
        return 0;
    }

    return stack->bufferSize;
}

void hash_stack_clear(hash_stack_t *stack) {
    ASSERT_ERROR(stack, TAG, "NULL stack") {
        return;
    }

    stack->size = 0;
}

void hash_stack_clear_hard(hash_stack_t *stack) {
    ASSERT_ERROR(stack, TAG, "NULL stack") {
        return;
    }

    HASH_STACK_CLEAR(stack);

    stack->size = 0;
    stack->levels = 0;
    stack->data = NULL;
}

static void _hash_stack_clear(const alloc_t *alloc, void *data, uint32_t size, uint32_t level) {
    void **ptr = data;
    level--;

    if (level == 0) {
        for (uint32_t i = 0; i < size; i++) {
            alloc_free(alloc, ptr[i]);
        }
    } else {
        for (uint32_t i = 0; i < size; i++) {
            _hash_stack_clear(alloc, ptr[i], size, level);
        }
    }

    alloc_free(alloc, data);
}