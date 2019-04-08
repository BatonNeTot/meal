#include "meal/list_pool.h"

#include "meal/platform.h"
#include "meal/assert.h"
#include "meal/def.h"


#define TAG "List Pool"

typedef struct block_t block_t;
typedef struct node_t node_t;

typedef struct block_t {
    block_t *next;
    void *data;
} block_t;

typedef struct node_t {
    node_t *next;
    void_t value;
} node_t;

typedef struct list_pool_t {
    const alloc_t *alloc;
    uint32_t typeSize;
    uint32_t bufferSize;
    block_t *header;
    node_t *freeTail;
} list_pool_t;


list_pool_t *list_pool_init_via(const alloc_t *alloc, uint32_t typeSize, uint32_t bufferSize) {
    ASSERT_ERROR(typeSize, TAG, "TypeSize must be more than 0: typeSize = %d", typeSize) {
        return NULL;
    }

    ASSERT_ERROR(bufferSize, TAG, "BufferSize must be more than 0: bufferSize = %d", bufferSize) {
        return NULL;
    }

    list_pool_t *pool = alloc_malloc(alloc, sizeof(list_pool_t));

    ASSERT_ERROR(pool, TAG, "Can't allocate memory for pool") {
        return NULL;
    }

    pool->alloc = alloc;
    pool->typeSize = (typeSize + (WSB - 1)) & ~(WSB - 1);
    pool->bufferSize = bufferSize;
    pool->header = NULL;
    pool->freeTail = NULL;

    return pool;
}

void list_pool_term(list_pool_t *pool) {
    ASSERT_ERROR(pool, TAG, "NULL pool") {
        return;
    }

    block_t *header = pool->header;
    while (header) {
        block_t *tmp = header;
        header = header->next;
        alloc_free(pool->alloc, tmp->data);
        alloc_free(pool->alloc, tmp);
    }
    alloc_free(pool->alloc, pool);
}

void *list_pool_get(list_pool_t *pool) {
    ASSERT_ERROR(pool, TAG, "NULL pool") {
        return NULL;
    }

    if (!pool->freeTail) {
        block_t *header = alloc_malloc(pool->alloc, sizeof(block_t));

        ASSERT_ERROR(header, TAG, "Can't allocate memory for pool data") {
            return NULL;
        }

        const uint32_t nodeSize = sizeof(node_t) + pool->typeSize;
        void *data = alloc_malloc(pool->alloc, nodeSize * pool->bufferSize);

        ASSERT_ERROR(data, TAG, "Can't allocate memory for pool data header") {
            alloc_free(pool->alloc, header);
            return NULL;
        }

        header->next = pool->header;
        pool->header = header;

        header->data = data;
        pool->freeTail = data;

        for (uint32_t i = pool->bufferSize - 1; i > 0; i--) {
            void *next = data + nodeSize;
            (*(node_t *)data).next = next;
            data = next;
        }

        (*(node_t *)data).next = NULL;
    }

    node_t *new = pool->freeTail;
    pool->freeTail = new->next;

    return &new->value;
}

bool list_pool_has(list_pool_t *pool, void *ptr) {
    ASSERT_ERROR(pool, TAG, "NULL pool") {
        return false;
    }

    ASSERT_ERROR(ptr, TAG, "NULL data") {
        return false;
    }

    block_t *block = pool->header;

    const uint32_t blockSize = (sizeof(node_t) + pool->typeSize) * pool->bufferSize;
    while (block) {
        if (block->data <= ptr && block->data + blockSize > ptr) {
            return true;
        }
    }

    return false;
}

void list_pool_free(list_pool_t *pool, void *ptr) {
    ASSERT_ERROR(pool, TAG, "NULL pool") {
        return;
    }

    ASSERT_ERROR(ptr, TAG, "NULL data") {
        return;
    }

#ifdef DEBUG
    block_t *block = pool->header;

    const uint32_t blockSize = (sizeof(node_t) + pool->typeSize) * pool->bufferSize;
    while (block) {
        if (block->data <= ptr && block->data + blockSize > ptr) {
            goto check_end;
        }
    }
    ASSERT_ERROR(false, TAG, "Ptr does not belong to pool") {};
    return;

    check_end:;
#endif

    node_t *node = (node_t *)ptr - 1;

    node->next = pool->freeTail;
    pool->freeTail = node;
}
