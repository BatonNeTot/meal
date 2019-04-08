#include "meal/memory.h"

#define INT_SIZE 4

void mem_copy(void *dst, const void *src, uint32_t size) {
    // May be work, must be better. need to check
    if (size) {
        if (dst < src) {
            for (; size >= INT_SIZE; dst += INT_SIZE, src += INT_SIZE, size -= INT_SIZE) {
                *(uint32_t *) dst = *(uint32_t *) src;
            }
            for (int i = 0; i < size; i++) {
                ((uint8_t *) dst)[i] = ((uint8_t *) src)[i];
            }
        } else if (dst > src) {
            const uint32_t mask = INT_SIZE - 1;
            while (size & mask) {
                size--;
                ((uint8_t *) dst)[size] = ((uint8_t *) src)[size];
            }
            size /= INT_SIZE;
            while (size) {
                size--;
                ((uint32_t *) dst)[size] = ((uint32_t *) src)[size];
            }
        }
    }

//    if (size & -4) {
//        int count = size >> 2;
//        for (int i = 0; i < count; i++) {
//            *(int32_t *)dst = *(int32_t *)src;
//            dst += 4;
//            src += 4;
//        }
//        size &= 3;
//    }
//    for (int i = 0; i < size; i++) {
//        ((char *)dst)[i] = ((char *)src)[i];
//    }
}