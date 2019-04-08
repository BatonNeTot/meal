#include "iter.h"

#include "meal/assert.h"

#define TAG "Iter"

iter_t *iter_copy(iter_t *iter) {
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return NULL;
    }

    return iter->funcs->copy(iter);
}

void iter_term(iter_t *iter) {
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return;
    }

    iter->funcs->term(iter);
}

void iter_next(iter_t *iter) {
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return;
    }

    iter->funcs->next(iter);
}

void iter_prev(iter_t *iter) {
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return;
    }

    iter->funcs->prev(iter);
}

void *iter_value(iter_t *iter) {
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return NULL;
    }

    return iter->funcs->value(iter);
}

const_iter_t *const_iter_copy(const_iter_t *iter) {
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return NULL;
    }

    return iter->funcs->copy(iter);
}

void const_iter_term(const_iter_t *iter) {
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return;
    }

    iter->funcs->term(iter);
}

void const_iter_next(const_iter_t *iter) {
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return;
    }

    iter->funcs->next(iter);
}

void const_iter_prev(const_iter_t *iter) {
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return;
    }

    iter->funcs->prev(iter);
}

const void *const_iter_value(const_iter_t *iter) {
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return NULL;
    }

    return iter->funcs->value(iter);
}