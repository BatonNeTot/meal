//
// Created by KILLdon on 17.03.2019.
//

#include "meal/alloc.h"

#include "meal/assert.h"

#include <stdlib.h>

#define TAG "Alloc"

typedef struct alloc_t {
    const alloc_t *alloc;
    const alloc_funcs_t *funcs;
    void *data;
} alloc_t;


alloc_t *alloc_init_via(const alloc_t *alloc, const alloc_funcs_t *funcs, void *data) {
    ASSERT_ERROR(funcs, TAG, "NULL functions") {
        return NULL;
    }

    alloc_t *result = alloc_malloc(alloc, sizeof(alloc_t));

    ASSERT_ERROR(result, TAG, "Can't allocate memory for allocator") {
        return NULL;
    }

    result->alloc = alloc;
    result->funcs = funcs;
    result->data = data;

    return result;
}

void alloc_term(alloc_t *alloc) {
    ASSERT_ERROR(alloc, TAG, "NULL allocator") {
        return;
    }

    alloc_free(alloc->alloc, alloc);
}


#define MALLOC(alloc, size)\
ASSERT_ERROR(size, TAG, "Size must be more than 0: size = %d", size){\
return NULL;\
}\
if (alloc) {\
    return alloc->funcs->malloc(size, alloc->data);\
} else {\
    return malloc(size);\
}

#define FREE(alloc, ptr)\
if (alloc) {\
    alloc->funcs->free(ptr, alloc->data);\
} else {\
    free(ptr);\
}

void *alloc_malloc(const alloc_t *alloc, uint32_t size) {
    MALLOC(alloc, size)
}

void *alloc_realloc(const alloc_t *alloc, void *ptr, uint32_t size) {
//    if (!ptr) {
//        MALLOC(alloc, size)
//    } else if (!size) {
//        FREE(alloc, ptr)
//        return NULL;
//    } else
    if (alloc) {
        return alloc->funcs->realloc(ptr, size, alloc->data);
    } else {
        return realloc(ptr, size);
    }
}

void alloc_free(const alloc_t *alloc, void *ptr) {
    FREE(alloc, ptr)
}
