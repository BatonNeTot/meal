#include "meal/cached_alloc.h"

#include "meal/rb_tree.h"
#include "meal/assert.h"
#include "meal/macros.h"
#include "meal/math.h"
#include "meal/memory.h"

#include <stdbool.h>

#define TAG "Cached Alloc"

#define BUFFER_SIZE 16

static void *_cached_alloc_malloc(uint32_t size, void *alloc);

static void *_cached_alloc_realloc(void *ptr, uint32_t size, void *alloc);

static void _cached_alloc_free(void *ptr, void *alloc);

static const alloc_funcs_t alloc_funcs = {
        _cached_alloc_malloc,
        _cached_alloc_realloc,
        _cached_alloc_free
};

typedef struct block_t {
    void *address;
    uint32_t size;
    bool inUse;
    bool isLast;
} block_t;

#define BLOCK(ptr) ((block_t *)ptr)

typedef struct chunk_t {
    uint32_t size;
    rb_tree_t *blockTree;
    void *chunk;
} chunk_t;

#define CHUNK(ptr) ((chunk_t *)ptr)

typedef struct empty_t {
    chunk_t *chunk;
    block_t *block;
} empty_t;

#define EMPTY(ptr) ((empty_t *)ptr)

typedef struct cached_alloc_t {
    const alloc_t *alloc;
    alloc_t *asAlloc;
    rb_tree_t *chunkTree;
    rb_tree_t *emptyTree;
    uint32_t bufferSize;
} cached_alloc_t;


static int32_t compareChunk(const void *left, const void *right) {
    return IN_RANGE(CHUNK(left)->chunk, CHUNK(right)->chunk, CHUNK(right)->chunk + CHUNK(right)->size);
}

static int32_t compareBlock(const void *left, const void *right) {
    return IN_RANGE(BLOCK(left)->address, BLOCK(right)->address, BLOCK(right)->address + BLOCK(right)->size);
    //return SIGN(BLOCK(left)->address - BLOCK(right)->address);
}

static int32_t compareEmpty(const void *left, const void *right) {
    if (EMPTY(left)->block->size == EMPTY(right)->block->size) {
        return CMPR(EMPTY(left)->block->address, EMPTY(right)->block->address);
    } else {
        return FTERN(EMPTY(left)->block->size < EMPTY(right)->block->size, -1, 1);
    }
}


cached_alloc_t *cached_alloc_init_via(const alloc_t *alloc, uint32_t bufferSize) {
    ASSERT_ERROR(bufferSize > 0, TAG, "Buffer size must be more than 0: bufferSize = %d", bufferSize) {
        return NULL;
    }

    cached_alloc_t *result = alloc_malloc(alloc, sizeof(cached_alloc_t));

    ASSERT_ERROR(result, TAG, "Can't allocate memory for alloc") {
        return NULL;
    }

    result->alloc = alloc;
    result->asAlloc = alloc_init_via(alloc, &alloc_funcs, result);

    ASSERT_ERROR(result->asAlloc, TAG, "Can't allocate memory for alloc wrap") {
        alloc_free(alloc, result);
        return NULL;
    }

    result->chunkTree = rb_tree_init_via(alloc, compareChunk, sizeof(chunk_t), BUFFER_SIZE);

    ASSERT_ERROR(result->chunkTree, TAG, "Can't allocate memory for chunk tree") {
        alloc_term(result->asAlloc);
        alloc_free(alloc, result);
        return NULL;
    }

    result->emptyTree = rb_tree_init_via(alloc, compareEmpty, sizeof(empty_t), BUFFER_SIZE);

    ASSERT_ERROR(result->emptyTree, TAG, "Can't allocate memory for block tree") {
        rb_tree_term(result->chunkTree);
        alloc_term(result->asAlloc);
        alloc_free(alloc, result);
        return NULL;
    }

    result->bufferSize = (bufferSize + (WSB - 1u)) & ~(WSB - 1u);

    return result;
}

static void _cached_alloc_term_tree(void *ptr, void *data) {
    alloc_free((alloc_t *)data, CHUNK(ptr)->chunk);
    rb_tree_term(CHUNK(ptr)->blockTree);
}

void cached_alloc_term(cached_alloc_t *alloc) {
    ASSERT_ERROR(alloc, TAG, "NULL alloc") {
        return;
    }

    rb_tree_term(alloc->emptyTree);
    rb_tree_foreach(alloc->chunkTree, _cached_alloc_term_tree, NULL);
    rb_tree_term(alloc->chunkTree);
    alloc_term(alloc->asAlloc);

    alloc_free(alloc->alloc, alloc);
}

const alloc_t *cached_alloc_as_alloc(cached_alloc_t *alloc) {
    ASSERT_ERROR(alloc, TAG, "NULL alloc") {
        return NULL;
    }

    return alloc->asAlloc;
}

#define SPLIT_BLOCK(emptyTree, _chunk, _block, _size)\
do {\
    /*Cut end of a block as empty; Note: next node must non exist or be in use*/\
    block_t newBlock = {_block->address + _size, _block->size - _size, false, _block->isLast};\
    \
    _block->size = _size;\
    _block->isLast = false;\
    \
    block_t *resultBlock = rb_tree_insert(_chunk->blockTree, &newBlock);\
    \
    empty_t emptyBlock = {_chunk, resultBlock};\
    rb_tree_insert(emptyTree, &emptyBlock);\
} while(0)

#define MALLOC_IMPL(_alloc, _size, resultAddress)\
do {\
    _size = (_size + (WSB - 1u)) & ~(WSB - 1u);\
\
    block_t targetTmp = {0, _size};\
    empty_t targetEmpty = {NULL, &targetTmp};\
    empty_t target;\
\
    if (!rb_tree_remove_min(_alloc->emptyTree, &targetEmpty, &target)) {\
        /*Free block wasn't found because of missing or suitable size*/\
        /*Create new chunk and provide block from it*/\
        /*Note: we need to know that needed size can be achieved,*/\
        /*Checked that before*/\
        void *data = alloc_malloc(_alloc->alloc, _alloc->bufferSize);\
        ASSERT_ERROR(data, TAG, "Can't allocate memory for chunk data") {\
            return NULL;\
        }\
\
        rb_tree_t *tree = rb_tree_init_via(_alloc->alloc, compareBlock, sizeof(block_t), BUFFER_SIZE);\
        ASSERT_ERROR(tree, TAG, "Can't allocate memory for chunk tree") {\
            alloc_free(_alloc->alloc, data);\
            return NULL;\
        }\
\
        chunk_t chunk = {_alloc->bufferSize, tree, data};\
        target.chunk = rb_tree_insert(_alloc->chunkTree, &chunk);\
        ASSERT_ERROR(target.chunk, TAG, "Can't insert new chunk in tree") {\
            rb_tree_term(tree);\
            alloc_free(_alloc->alloc, data);\
            return NULL;\
        }\
\
        block_t block = {data, _alloc->bufferSize, false, true};\
        target.block = rb_tree_insert(tree, &block);\
        ASSERT_ERROR(target.block, TAG, "Can't insert new block in chunk tree") {\
            rb_tree_remove(_alloc->chunkTree, &chunk, NULL);\
            rb_tree_term(tree);\
            alloc_free(_alloc->alloc, data);\
            return NULL;\
        }\
    }\
\
    if (target.block->size > _size) {\
        /*Cut unnecessary*/\
        SPLIT_BLOCK(_alloc->emptyTree, target.chunk, target.block, _size);\
    }\
\
    target.block->inUse = true;\
\
    resultAddress = target.block->address;\
} while(0)

#define FREE_BLOCK(emptyTree, _chunk, prev, current, next)\
do {\
    if (next && !next->inUse) {\
        /*Merge current with next if exist*/\
        current->isLast = next->isLast;\
\
        empty_t tmp = {_chunk, next};\
        rb_tree_remove(emptyTree, &tmp, NULL);\
\
        rb_tree_remove(_chunk->blockTree, next, NULL);\
        current->size += next->size;\
    }\
\
    if (prev && !prev->inUse) {\
        /*Merge current with prev if exist*/\
        prev->isLast = current->isLast;\
\
        empty_t tmp = {_chunk, prev};\
        rb_tree_remove(emptyTree, &tmp, NULL);\
\
        rb_tree_remove(_chunk->blockTree, current, NULL);\
        prev->size += current->size;\
\
        current = prev;\
    } else {\
        current->inUse = false;\
    }\
\
    empty_t tmp = {_chunk, current};\
    rb_tree_insert(emptyTree, &tmp);\
} while (0)

#define REALLOC_IMPL(_alloc, ptr, _size)\
do {\
    _size = (_size + (WSB - 1u)) & ~(WSB - 1u);\
\
    chunk_t chunkTmp = {0, NULL, ptr};\
    chunk_t *chunk = rb_tree_find(_alloc->chunkTree, &chunkTmp);\
    ASSERT_ERROR(chunk, TAG, "Can't find ptr in alloc") {\
        return NULL;\
    }\
\
    block_t blockTmp = {ptr};\
    iter_t *blockIter = rb_tree_find_iter(chunk->blockTree, &blockTmp);\
    ASSERT_ERROR(blockIter, TAG, "Can't find ptr in alloc") {\
        iter_term(blockIter);\
        return NULL;\
    }\
\
    block_t *current = iter_value(blockIter);\
    if (_size == current->size) {\
        return ptr;\
    }\
\
    block_t *next = NULL;\
    if (!current->isLast) {\
        iter_t *blockIterCopy = iter_copy(blockIter);\
\
        iter_next(blockIterCopy);\
        next = iter_value(blockIterCopy);\
\
        iter_term(blockIterCopy);\
    }\
\
    if (_size < current->size) {\
        /*Target size smaller, shrinking*/\
        if (!next || next->inUse) {\
            /*Next is unusable, cut*/\
            SPLIT_BLOCK(_alloc->emptyTree, chunk, current, _size);\
        } else {\
            /*Next can be used, move border*/\
            empty_t tmp = {chunk, next};\
            rb_tree_remove(_alloc->emptyTree, &tmp, NULL);\
\
            const uint32_t diff = current->size - _size;\
            next->address -= diff;\
            next->size += diff;\
\
            rb_tree_insert(_alloc->emptyTree, &tmp);\
\
            current->size -= diff;\
        }\
        iter_term(blockIter);\
\
        return ptr;\
    } else if (next && !next->inUse && current->size + next->size >= _size) {\
        /*Target size bigger*/\
        /*Can merge with next*/\
        empty_t tmp = {chunk, next};\
        rb_tree_remove(_alloc->emptyTree, &tmp, NULL);\
\
        if (next->size > _size - current->size) {\
            /*Next too big, move border*/\
            current->size = _size;\
\
            const uint32_t diff = _size - current->size;\
            next->address += diff;\
            next->size -= diff;\
\
            rb_tree_insert(_alloc->emptyTree, &tmp);\
        } else {\
            /*Next perfectly feet, merge*/\
            current->isLast = next->isLast;\
\
            rb_tree_remove(chunk->blockTree, next, NULL);\
\
            current->size = _size;\
        }\
        iter_term(blockIter);\
\
        return ptr;\
    } else {\
        /*Can not merge with next*/\
        /*Freeing current as we know we can't damage data now*/\
        /*After that allocate new block and copy data*/\
        block_t *prev = NULL;\
        if (chunk->chunk != current->address) {\
            iter_prev(blockIter);\
            prev = iter_value(blockIter);\
        }\
        iter_term(blockIter);\
\
        FREE_BLOCK(_alloc->emptyTree, chunk, prev, current, next);\
\
        void *newPtr;\
        MALLOC_IMPL(_alloc, _size, newPtr);\
\
        mem_copy(newPtr, ptr, current->size);\
\
        return newPtr;\
    }\
} while(0)

#define FREE_IMPL(_alloc, ptr, _returnValue)\
do {\
    chunk_t chunkTmp = {0, NULL, ptr};\
    chunk_t *chunk = rb_tree_find(_alloc->chunkTree, &chunkTmp);\
    ASSERT_ERROR(chunk, TAG, "Can't find ptr in alloc") {\
        return _returnValue;\
    }\
\
    block_t blockTmp = {ptr};\
    iter_t *blockIter = rb_tree_find_iter(chunk->blockTree, &blockTmp);\
    ASSERT_ERROR(blockIter, TAG, "Can't find ptr in alloc") {\
        iter_term(blockIter);\
        return _returnValue;\
    }\
\
    block_t *current = iter_value(blockIter);\
    block_t *next = NULL;\
    block_t *prev = NULL;\
\
    if (!current->isLast) {\
        if (chunk->chunk != current->address) {\
            iter_t *blockIterCopy = iter_copy(blockIter);\
\
            iter_next(blockIterCopy);\
            next = iter_value(blockIterCopy);\
\
            iter_term(blockIterCopy);\
\
            iter_prev(blockIter);\
            prev = iter_value(blockIter);\
        } else {\
            iter_next(blockIter);\
            next = iter_value(blockIter);\
        }\
    } else {\
        if (chunk->chunk != current->address) {\
            iter_prev(blockIter);\
            prev = iter_value(blockIter);\
        }\
    }\
    iter_term(blockIter);\
\
    FREE_BLOCK(_alloc->emptyTree, chunk, prev, current, next);\
    return _returnValue;\
} while(0)

#define ALLOC(_alloc, _size)\
do {\
    ASSERT_ERROR(_alloc, TAG, "NULL alloc") {\
        return NULL;\
    }\
\
    if (!_size) {\
        log_warning(TAG, "Trying to allocate zero size memory");\
        return NULL;\
    }\
\
    if (_size > _alloc->bufferSize) {\
        log_warning(TAG, "Trying to allocate memory more than buffer: "\
                         "bufferSize = %d; size = %d", _alloc->bufferSize, _size);\
        return NULL;\
    }\
\
    void *newPtr;\
    MALLOC_IMPL(_alloc, _size, newPtr);\
    return newPtr;\
} while(0)

#define REALLOC(_alloc, ptr, _size)\
do {\
    ASSERT_ERROR(_alloc, TAG, "NULL alloc") {\
        return NULL;\
    }\
\
    if (!ptr) {\
        ASSERT_ERROR(_size, TAG, "NULL ptr and zero size") {\
            return NULL;\
        }\
\
        void *newPtr;\
        MALLOC_IMPL(_alloc, _size, newPtr);\
        return newPtr;\
    }\
\
    if (!_size) {\
        FREE_IMPL(_alloc, ptr, NULL);\
        return NULL;\
    }\
\
    if (_size > _alloc->bufferSize) {\
        log_warning(TAG, "Trying to allocate memory more than buffer: "\
                         "bufferSize = %d; size = %d", _alloc->bufferSize, _size);\
        return NULL;\
    }\
\
    REALLOC_IMPL(_alloc, ptr, _size);\
} while(0)

#define FREE(_alloc, ptr)\
do {\
    ASSERT_ERROR(_alloc, TAG, "NULL alloc") {\
        return;\
    }\
\
    ASSERT_ERROR(ptr, TAG, "NULL ptr") {\
        return ;\
    }\
\
    FREE_IMPL(_alloc, ptr,);\
} while(0)

void *cached_alloc_malloc(cached_alloc_t *alloc, uint32_t size) {
    ALLOC(alloc, size);
}

void *cached_alloc_realloc(cached_alloc_t *alloc, void *ptr, uint32_t size) {
    REALLOC(alloc, ptr, size);
}

void cached_alloc_free(cached_alloc_t *alloc, void *ptr) {
    FREE(alloc, ptr);
}

static void *_cached_alloc_malloc(uint32_t size, void *data) {
    ALLOC(((cached_alloc_t *)data), size);
}

static void *_cached_alloc_realloc(void *ptr, uint32_t size, void *data) {
    REALLOC(((cached_alloc_t *)data), ptr, size);
}

static void _cached_alloc_free(void *ptr, void *data) {
    FREE(((cached_alloc_t *)data), ptr);
}
