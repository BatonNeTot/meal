#include "meal/rb_tree.h"

#include "meal/list_pool.h"
#include "meal/assert.h"
#include "meal/memory.h"
#include "meal/macros.h"
#include "meal/math.h"
#include "iter.h"

#define TAG "RB Tree"

typedef struct node_t node_t;

typedef enum stackFlag {
    WAS_NONE,
    WAS_LEFT,
    WAS_RIGHT,
} stackFlag;

typedef enum color {
    RED,
    BLACK,
} color;

typedef struct node_t {
    color color;
    node_t *parent;
    node_t *left;
    node_t *right;
    void_t data;
} node_t;

#define NODE(ptr) ((node_t *)ptr)

typedef struct rb_tree_t {
    const alloc_t *alloc;
    cmp_f compare;
    list_pool_t *nodePool;
    list_pool_t *iterPool;
    node_t *root;
    uint32_t typeSize;
    uint32_t size;
} rb_tree_t;

typedef struct iter_box_t {
    list_pool_t *pool;
    node_t *node;
} iter_box_t;

#define ITER(ptr) ((iter_box_t *)ptr)

rb_tree_t *rb_tree_init_via(const alloc_t *alloc, cmp_f compare, size_t typeSize, size_t bufferSize) {
    ASSERT_ERROR(compare, TAG, "NULL comparator") {
        return NULL;
    }

    ASSERT_ERROR(typeSize, TAG, "TypeSize must be more than 0: typeSize = %d", typeSize) {
        return NULL;
    }

    ASSERT_ERROR(bufferSize > 1, TAG, "TypeSize must be more than 1: bufferSize = %d", bufferSize) {
        return NULL;
    }

    rb_tree_t *tree = alloc_malloc(alloc, sizeof(rb_tree_t));

    ASSERT_ERROR(tree, TAG, "Can't allocate memory for tree") {
        return NULL;
    }

    tree->alloc = alloc;
    tree->compare = compare;
    tree->nodePool = list_pool_init_via(alloc, sizeof(node_t) + typeSize, bufferSize);

    ASSERT_ERROR(tree->nodePool, TAG, "Can't allocate memory for pool") {
        alloc_free(alloc, tree);
        return NULL;
    }

    tree->iterPool = list_pool_init_via(alloc, sizeof(iter_t) + sizeof(iter_box_t), bufferSize);

    ASSERT_ERROR(tree->iterPool, TAG, "Can't allocate memory for pool") {
        list_pool_term(tree->nodePool);
        alloc_free(alloc, tree);
        return NULL;
    }

    tree->root = NULL;
    tree->typeSize = typeSize;
    tree->size = 0;


    return tree;
}

void rb_tree_term(rb_tree_t *tree) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return;
    }

    list_pool_term(tree->nodePool);

    alloc_free(tree->alloc, tree);
}

#define RB_TREE_NODE_NEW(tree, node, ptr)\
do {\
    node = list_pool_get(tree->nodePool);\
    ASSERT_ERROR(node, TAG, "Can't allocate memory for tree node") {\
        return NULL;\
    }\
    \
    node->parent = NULL;\
    node->left = NULL;\
    node->right = NULL;\
    \
    mem_copy(&node->data, ptr, tree->typeSize);\
} while (0)

#define TO_ROOT(tree, node)\
do {\
    tree->root = node;\
    node && (node->parent = NULL);\
} while(0)

#define TO_SIDE(father, side, node)\
do {\
    father->side = node;\
    node && (node->parent = father);\
} while(0)


#define NEXT(node) \
do {\
    if (node->right) {\
        node = node->right;\
        while (node->left) {\
            node = node->left;\
        }\
    } else {\
        while (node->parent && node->parent->right == node) {\
            node = node->parent;\
        }\
        node = node->parent;\
    }\
} while(0)

#define PREV(node) \
do {\
    if (node->left) {\
        node = node->left;\
        while (node->right) {\
            node = node->right;\
        }\
    } else {\
        while (node->parent && node->parent->left == node) {\
            node = node->parent;\
        }\
        node = node->parent;\
    }\
} while (0)

static void _rb_tree_iter_next(iter_t *iter) {
    if (ITER(&iter->data)->node) {
        NEXT(ITER(&iter->data)->node);
    }
}
static void _rb_tree_iter_prev(iter_t *iter) {
    if (ITER(&iter->data)->node) {
        PREV(ITER(&iter->data)->node);
    }
}

static void *_rb_tree_iter_value(iter_t *iter) {
    return ITER(&iter->data)->node ? &ITER(&iter->data)->node->data : NULL;
}

static iter_t *_rb_tree_iter_copy(iter_t *iter);

static void _rb_tree_iter_term(iter_t *iter) {
    list_pool_free(ITER(&iter->data)->pool, iter);
}

static const iter_funcs_t _iter_funcs = {
        _rb_tree_iter_next,
        _rb_tree_iter_prev,
        _rb_tree_iter_value,
        _rb_tree_iter_copy,
        _rb_tree_iter_term
};

#define CREATE_ITER(iter, _pool, _node)\
do {\
    iter = list_pool_get(_pool);\
    ASSERT_ERROR(iter, TAG, "Can't allocate memory for tree iterator") {\
        return NULL;\
    }\
    iter->funcs = &_iter_funcs;\
    ITER(&iter->data)->pool = _pool;\
    ITER(&iter->data)->node = _node;\
} while (0)

static iter_t *_rb_tree_iter_copy(iter_t *iter) {
    iter_t *newIter;
    CREATE_ITER(newIter, ITER(&iter->data)->pool, ITER(&iter->data)->node);
    return newIter;
}

iter_t *rb_tree_iter_tmp(rb_tree_t *tree, const void *ptr) {

}

#define RB_TREE_INSERT_BALANCE(tree, target) \
while (target->color == RED) {\
    if (!target->parent) {\
        target->color = BLACK;\
        return NULL;\
    }\
\
    node_t *father = target->parent;\
    node_t *brother = (node_t *) FTERNP(father->left == target, father->right, father->left);\
\
    if (!brother || brother->color == BLACK) {\
        node_t *grand = father->parent;\
        \
        if (father->left == target) {\
            TO_SIDE(father, left, target->right);\
            TO_SIDE(target, right, father);\
        } else {\
            TO_SIDE(father, right, target->left);\
            TO_SIDE(target, left, father);\
        }\
\
        father->color = RED;\
        target->color = BLACK;\
\
        if (grand) {\
            if (grand->left == father) {\
                TO_SIDE(grand, left, target);\
            } else {\
                TO_SIDE(grand, right, target);\
            }\
        } else {\
            TO_ROOT(tree, target);\
        }\
        break;\
    }\
\
    target->color = BLACK;\
    brother->color = BLACK;\
\
    if (father->parent) {\
        father->color = RED;\
        target = father->parent;\
    } else {\
        break;\
    }\
}

void *rb_tree_insert(rb_tree_t *tree, const void *ptr) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return NULL;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return NULL;
    }

    if (!tree->root) {
        node_t *node;
        RB_TREE_NODE_NEW(tree, node, ptr);

        node->color = BLACK;

        TO_ROOT(tree, node);
        tree->size++;

        return &node->data;
    } else {
        node_t *tmp = tree->root;

        while (1) {
            int32_t cmpr = tree->compare(ptr, &tmp->data);
            try_again:
            switch (cmpr) {
                case -1: {
                    if (!tmp->left) {
                        node_t *node;
                        RB_TREE_NODE_NEW(tree, node, ptr);

                        node->color = RED;

                        TO_SIDE(tmp, left, node);
                        tree->size++;

                        RB_TREE_INSERT_BALANCE(tree, tmp)
                        return &node->data;\

                    }
                    tmp = tmp->left;
                    break;
                }
                case 1: {
                    if (!tmp->right) {
                        node_t *node;
                        RB_TREE_NODE_NEW(tree, node, ptr);

                        node->color = RED;

                        TO_SIDE(tmp, right, node);
                        tree->size++;

                        RB_TREE_INSERT_BALANCE(tree, tmp)
                        return &node->data;\

                    }
                    tmp = tmp->right;\
                    break;
                }
                case 0: {
                    return NULL;
                }
                default: {
                    cmpr = SIGN(cmpr);
                    goto try_again;
                }
            }
        }
    }
}

iter_t *rb_tree_insert_iter(rb_tree_t *tree, const void *ptr) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return NULL;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return NULL;
    }

    if (!tree->root) {
        node_t *node;
        RB_TREE_NODE_NEW(tree, node, ptr);

        node->color = BLACK;

        TO_ROOT(tree, node);
        tree->size++;

        iter_t *iter;
        CREATE_ITER(iter, tree->iterPool, node);
        return iter;
    } else {
        node_t *tmp = tree->root;

        while (1) {
            int32_t cmpr = tree->compare(ptr, &tmp->data);
            try_again:
            switch (cmpr) {
                case -1: {
                    if (!tmp->left) {
                        node_t *node;
                        RB_TREE_NODE_NEW(tree, node, ptr);

                        node->color = RED;

                        TO_SIDE(tmp, left, node);
                        tree->size++;

                        RB_TREE_INSERT_BALANCE(tree, tmp)

                        iter_t *iter;
                        CREATE_ITER(iter, tree->iterPool, node);
                        return iter;

                    }
                    tmp = tmp->left;
                    break;
                }
                case 1: {
                    if (!tmp->right) {
                        node_t *node;
                        RB_TREE_NODE_NEW(tree, node, ptr);

                        node->color = RED;

                        TO_SIDE(tmp, right, node);
                        tree->size++;

                        RB_TREE_INSERT_BALANCE(tree, tmp)

                        iter_t *iter;
                        CREATE_ITER(iter, tree->iterPool, node);
                        return iter;

                    }
                    tmp = tmp->right;\
                    break;
                }
                case 0: {
                    return NULL;
                }
                default: {
                    cmpr = SIGN(cmpr);
                    goto try_again;
                }
            }
        }
    }
}

void *rb_tree_find(rb_tree_t *tree, const void *ptr) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return NULL;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return NULL;
    }

    node_t *tmp = tree->root;
    while (tmp) {
        int32_t cmpr = tree->compare(ptr, &tmp->data);
        try_again:
        switch (cmpr) {
            case -1: {
                if (tmp->left) {
                    tmp = tmp->left;
                } else {
                    return NULL;
                }
                break;
            }
            case 1: {
                if (tmp->right) {
                    tmp = tmp->right;
                } else {
                    return NULL;
                }
                break;
            }
            case 0: {
                return &tmp->data;
            }
            default: {
                cmpr = SIGN(cmpr);
                goto try_again;
            }
        }
    }

    return NULL;
}

void *rb_tree_min(rb_tree_t *tree, const void *ptr) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return NULL;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return NULL;
    }

    node_t *tmp = tree->root;
    while (tmp) {
        int32_t cmpr = tree->compare(ptr, &tmp->data);
        try_again:
        switch (cmpr) {
            case -1: {
                if (tmp->left) {
                    tmp = tmp->left;
                } else {
                    goto found_it;
                }
                break;
            }
            case 1: {
                if (tmp->right) {
                    tmp = tmp->right;
                } else {
                    NEXT(tmp);

                    if (!tmp) {
                        return NULL;
                    } else {
                        goto found_it;
                    }
                }
                break;
            }
            case 0: {
                found_it:
                return &tmp->data;
            }
            default: {
                cmpr = SIGN(cmpr);
                goto try_again;
            }
        }
    }

    return NULL;
}

void *rb_tree_max(rb_tree_t *tree, const void *ptr) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return NULL;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return NULL;
    }

    node_t *tmp = tree->root;
    while (tmp) {
        int32_t cmpr = tree->compare(ptr, &tmp->data);
        try_again:
        switch (cmpr) {
            case -1: {
                if (tmp->left) {
                    tmp = tmp->left;
                } else {
                    PREV(tmp);

                    if (!tmp) {
                        return NULL;
                    } else {
                        goto found_it;
                    }
                }
                break;
            }
            case 1: {
                if (tmp->right) {
                    tmp = tmp->right;
                } else {
                    goto found_it;
                }
                break;
            }
            case 0: {
                found_it:
                return &tmp->data;
            }
            default: {
                cmpr = SIGN(cmpr);
                goto try_again;
            }
        }
    }

    return NULL;
}

void rb_tree_foreach(rb_tree_t *tree, action_f func, void *data) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return;
    }

    node_t *tmp = tree->root;
    if (tmp) {
        while (tmp->left) {
            tmp = tmp->left;
        }

        while (tmp) {
            func(&tmp->data, data);
            NEXT(tmp);
        }
    }
}

iter_t *rb_tree_iter(rb_tree_t *tree) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return NULL;
    }

    node_t *node = tree->root;

    while (node->left) {
        node = node->left;
    }

    iter_t *iter;
    CREATE_ITER(iter, tree->iterPool, node);

    return iter;
}

iter_t *rb_tree_find_iter(rb_tree_t *tree, const void *ptr) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return NULL;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return NULL;
    }

    node_t *tmp = tree->root;
    while (tmp) {
        int32_t cmpr = tree->compare(ptr, &tmp->data);
        try_again:
        switch (cmpr) {
            case -1: {
                if (tmp->left) {
                    tmp = tmp->left;
                } else {
                    return NULL;
                }
                break;
            }
            case 1: {
                if (tmp->right) {
                    tmp = tmp->right;
                } else {
                    return NULL;
                }
                break;
            }
            case 0: {
                iter_t *iter;
                CREATE_ITER(iter, tree->iterPool, tmp);
                return iter;
            }
            default: {
                cmpr = SIGN(cmpr);
                goto try_again;
            }
        }
    }

    return NULL;
}

iter_t *rb_tree_min_iter(rb_tree_t *tree, const void *ptr) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return NULL;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return NULL;
    }

    node_t *tmp = tree->root;
    while (tmp) {
        int32_t cmpr = tree->compare(ptr, &tmp->data);
        try_again:
        switch (cmpr) {
            case -1: {
                if (tmp->left) {
                    tmp = tmp->left;
                } else {
                    goto found_it;
                }
                break;
            }
            case 1: {
                if (tmp->right) {
                    tmp = tmp->right;
                } else {
                    NEXT(tmp);

                    if (!tmp) {
                        return NULL;
                    } else {
                        goto found_it;
                    }
                }
                break;
            }
            case 0: {
                found_it:;
                iter_t *iter;
                CREATE_ITER(iter, tree->iterPool, tmp);
                return iter;
            }
            default: {
                cmpr = SIGN(cmpr);
                goto try_again;
            }
        }
    }

    return NULL;
}

iter_t *rb_tree_max_iter(rb_tree_t *tree, const void *ptr) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return NULL;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return NULL;
    }

    node_t *tmp = tree->root;
    while (tmp) {
        int32_t cmpr = tree->compare(ptr, &tmp->data);
        try_again:
        switch (cmpr) {
            case -1: {
                if (tmp->left) {
                    tmp = tmp->left;
                } else {
                    PREV(tmp);

                    if (!tmp) {
                        return NULL;
                    } else {
                        goto found_it;
                    }
                }
                break;
            }
            case 1: {
                if (tmp->right) {
                    tmp = tmp->right;
                } else {
                    goto found_it;
                }
                break;
            }
            case 0: {
                found_it:;
                iter_t *iter;
                CREATE_ITER(iter, tree->iterPool, tmp);
                return iter;
            }
            default: {
                cmpr = SIGN(cmpr);
                goto try_again;
            }
        }
    }

    return NULL;
}


#define RB_TREE_REMOVE_BALANCE(tree, target)\
do {\
    node_t *toDelete = target;\
    node_t *father = target->parent;\
\
    /* If node is RED, it's definitely has no children, no need to balance */\
    if (target->color == RED) {\
        if (father) {\
            if (father->left == target) {\
                father->left = NULL;\
            } else {\
                father->right = NULL;\
            }\
        } else {\
            tree->root = NULL;\
        }\
    } else {\
        node_t *child = (node_t *) ((WST) target->left | (WST) target->right);\
        if (child) {\
            /* Has child, just passing child to replace node, no need to balance */\
            child->color = BLACK;\
\
            if (father) {\
                if (father->left == target) {\
                    TO_SIDE(father, left, child);\
                } else {\
                    TO_SIDE(father, right, child);\
                }\
            } else {\
                TO_ROOT(tree, child);\
            }\
        } else {\
            /* Node hasn't children, balancing */\
\
            if (father) {\
                if (father->left == target) {\
                    target = NULL;\
                    father->left = NULL;\
                    goto definitely_has_parent;\
                } else {\
                    target = NULL;\
                    father->right = NULL;\
                    goto definitely_has_parent;\
                }\
            } else {\
                tree->root = NULL;\
                goto done;\
            }\
            \
\
            check_iteration:\
\
            if (!target->parent) {\
                /* First case: Problem target is Root */\
                /* Just recolor */\
                /* Termination */\
\
                target->color = BLACK;\
\
                goto done;\
            } else {\
                father = target->parent;\
\
                definitely_has_parent:;\
\
                node_t *brother = (node_t *) FTERNP(father->left == target,\
                                                    father->right,\
                                                    father->left);\
\
                node_t *farNephew = (node_t *) FTERNP(father->left == target,\
                                                      brother->right,\
                                                      brother->left);\
\
                node_t *nearNephew = (node_t *) FTERNP(father->left == target,\
                                                       brother->left,\
                                                       brother->right);\
\
                if (father->color == BLACK) {\
                    if (brother->color == RED) {\
                        /* Second case: BLACK parent, RED brother */\
                        /* Turn with recolor */\
                        /* Go to "definitely_has_parent" */\
                        node_t *grand = father->parent;\
\
                        if (father->left == target) {\
                            TO_SIDE(father, right, nearNephew);\
                            TO_SIDE(brother, left, father);\
                        } else {\
                            TO_SIDE(father, left, nearNephew);\
                            TO_SIDE(brother, right, father);\
                        }\
\
                        father->color = RED;\
                        brother->color = BLACK;\
\
                        if (grand) {\
                            if (grand->left == father) {\
                                TO_SIDE(grand, left, brother);\
                            } else {\
                                TO_SIDE(grand, right, brother);\
                            }\
                        } else {\
                            TO_ROOT(tree, brother);\
                        }\
\
                        goto definitely_has_parent;\
                    } else {\
                        if (farNephew && farNephew->color == RED) {\
                            goto case_six;\
                        } else {\
                            if (!nearNephew || nearNephew->color == BLACK) {\
                                /* Third case: BLACK parent, BLACK nephews */\
                                /* Just recolor */\
                                /* To next loop iteration */\
\
                                brother->color = RED;\
                                target = father;\
\
                                goto check_iteration;\
                            } else {\
                                /* Fifth case: BLACK parent, RED far nephew */\
                                /* Turn with recolor */\
                                /* Go to case 6 */\
\
                                brother->color = RED;\
                                nearNephew->color = BLACK;\
\
                                if (father->left == target) {\
                                    TO_SIDE(father, right, nearNephew);\
                                    TO_SIDE(brother, left, nearNephew->right);\
                                    TO_SIDE(nearNephew, right, brother);\
\
                                    farNephew = brother;\
                                    brother = nearNephew;\
                                    nearNephew = nearNephew->left;\
\
                                } else {\
                                    TO_SIDE(father, left, nearNephew);\
                                    TO_SIDE(brother, right, nearNephew->left);\
                                    TO_SIDE(nearNephew, left, brother);\
\
                                    farNephew = brother;\
                                    brother = nearNephew;\
                                    nearNephew = nearNephew->right;\
                                }\
                                goto case_six;\
                            }\
                        }\
                    }\
                } else {\
                    if (!farNephew || farNephew->color == BLACK) {\
                        /* Forth case: RED parent, far nephew is BLACK */\
                        /* Just recolor */\
                        /* Termination */\
\
                        father->color = BLACK;\
                        brother->color = RED;\
\
                        goto done;\
                    } else {\
                        case_six:;\
                        /* Six case: No matter parent end near nephew, far nephew is RED */\
                        /* Turn with recolor */\
                        /* Termination */\
                        node_t *grand = father->parent;\
\
                        if (father->left == target) {\
                            TO_SIDE(father, right, nearNephew);\
                            TO_SIDE(brother, left, father);\
                        } else {\
                            TO_SIDE(father, left, nearNephew);\
                            TO_SIDE(brother, right, father);\
                        }\
\
                        farNephew->color = BLACK;\
                        brother->color = father->color;\
                        father->color = BLACK;\
\
                        if (grand) {\
                            if (grand->left == father) {\
                                TO_SIDE(grand, left, brother);\
                            } else {\
                                TO_SIDE(grand, right, brother);\
                            }\
                        } else {\
                            TO_ROOT(tree, brother);\
                        }\
\
                        goto done;\
                    }\
                }\
            }\
        }\
    }\
    done:\
    list_pool_free(tree->nodePool, toDelete);\
} while (0)

#define RB_TREE_REMOVE(tree, target) \
do {\
    /* We need our node has no or one child */\
    if (target->left && target->right) {\
        /* Our node has two children, finding donor */\
        node_t *father = target->parent;\
        node_t *donor = target;\
        \
        donor = donor->right;\
        while (donor->left) {\
            donor = donor->left;\
        }\
        node_t *fdonor = donor->parent;\
        \
        /* Changing target node with donor */\
        node_t bufferNode = *donor;\
        *donor = *target;\
        *target = bufferNode;\
        \
        donor->left->parent = donor;\
        \
        if (father) {\
            if (father->left == target) {\
                TO_SIDE(father, left, donor);\
            } else {\
                TO_SIDE(father, right, donor);\
            }\
        } else {\
            TO_ROOT(tree, donor);\
        }\
        \
        if (fdonor == target) {\
            if (donor->left == donor) {\
                TO_SIDE(donor, left, target);\
            } else {\
                TO_SIDE(donor, right, target);\
            }\
        } else {\
            if (fdonor->left == donor) {\
                TO_SIDE(fdonor, left, target);\
            } else {\
                TO_SIDE(fdonor, right, target);\
            }\
        }\
    }\
    \
    if (dst) {\
        mem_copy(dst, &target->data, tree->typeSize);\
    }\
    \
    RB_TREE_REMOVE_BALANCE(tree, target);\
} while (0)


bool rb_tree_remove(rb_tree_t *tree, const void *ptr, void *dst) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return false;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return false;
    }

    node_t *tmp = tree->root;
    while (tmp) {
        int32_t cmpr = tree->compare(ptr, &tmp->data);
        try_again:
        switch (cmpr) {
            case -1: {
                if (tmp->left) {
                    tmp = tmp->left;
                } else {
                    return false;
                }
                break;
            }
            case 1: {
                if (tmp->right) {
                    tmp = tmp->right;
                } else {
                    return false;
                }
                break;
            }
            case 0: {
                RB_TREE_REMOVE(tree, tmp);

                tree->size--;
                return true;
            }
            default: {
                cmpr = SIGN(cmpr);
                goto try_again;
            }
        }
    }

    return false;
}

bool rb_tree_remove_min(rb_tree_t *tree, const void *ptr, void *dst) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return false;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return false;
    }

    node_t *tmp = tree->root;
    while (tmp) {
        int32_t cmpr = tree->compare(ptr, &tmp->data);
        try_again:
        switch (cmpr) {
            case -1: {
                if (tmp->left) {
                    tmp = tmp->left;
                } else {
                    goto found_it;
                }
                break;
            }
            case 1: {
                if (tmp->right) {
                    tmp = tmp->right;
                } else {
                    NEXT(tmp);

                    if (!tmp) {
                        return NULL;
                    } else {
                        goto found_it;
                    }
                }
                break;
            }
            case 0: {
                found_it:
                RB_TREE_REMOVE(tree, tmp);

                tree->size--;
                return true;
            }
            default: {
                cmpr = SIGN(cmpr);
                goto try_again;
            }
        }
    }

    return false;
}

bool rb_tree_remove_max(rb_tree_t *tree, const void *ptr, void *dst) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return false;
    }

    ASSERT_ERROR(ptr, TAG, "NULL ptr") {
        return false;
    }

    node_t *tmp = tree->root;
    while (tmp) {
        int32_t cmpr = tree->compare(ptr, &tmp->data);
        try_again:
        switch (cmpr) {
            case -1: {
                if (tmp->left) {
                    tmp = tmp->left;
                } else {
                    PREV(tmp);

                    if (!tmp) {
                        return false;
                    } else {
                        goto found_it;
                    }
                }
                break;
            }
            case 1: {
                if (tmp->right) {
                    tmp = tmp->right;
                } else {
                    goto found_it;
                }
                break;
            }
            case 0: {
                found_it:
                RB_TREE_REMOVE(tree, tmp);

                tree->size--;
                return true;
            }
            default: {
                cmpr = SIGN(cmpr);
                goto try_again;
            }
        }
    }

    return false;
}

bool rb_tree_remove_iter(rb_tree_t *tree, iter_t *iter, void *dst) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return false;
    }
    ASSERT_ERROR(iter, TAG, "NULL iterator") {
        return false;
    }

    ASSERT_ERROR(list_pool_has(tree->nodePool, ITER(&iter->data)->node), TAG, "Node on iterator stack not belong to tree") {
        return false;
    }

    if (!ITER(&iter->data)->node) {
        return false;
    }

    RB_TREE_REMOVE(tree, ITER(&iter->data)->node);

    iter_term(iter);
    tree->size--;
    return true;
}

inline size_t rb_tree_size(rb_tree_t *tree) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return 0;
    }

    return tree->size;
}

void rb_tree_clear(rb_tree_t *tree) {
    ASSERT_ERROR(tree, TAG, "NULL tree") {
        return;
    }

    node_t *tmp = tree->root;
    while (tmp) {
        node_t *child = (node_t *)FTERNP(tmp->left != NULL, tmp->left, tmp->right);
        if (child) {
            tmp = child;
        } else {
            list_pool_free(tree->nodePool, tmp);
            tmp = tmp->parent;
        }
    }
}