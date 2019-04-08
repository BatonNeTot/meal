#ifndef MEAL_PRINT_H
#define MEAL_PRINT_H

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

typedef int32_t (*writer_f)(const char *buffer, uint32_t count, void *data);

int32_t wprintv(writer_f writer, void *data, const char *frmt, va_list args);
int32_t fprintv(FILE *stream, const char *frmt, va_list args);
int32_t sprintv(char *str, const char *frmt, va_list args);
int32_t snprintv(char *buffer, size_t size, const char *frmt, va_list args);

int32_t wprint(writer_f writer, void *data, const char *frmt, ...);
int32_t fprint(FILE *stream, const char *frmt, ...);
int32_t sprint(char *str, const char *frmt, ...);
int32_t snprint(char *buffer, size_t size, const char *frmt, ...);

const char *itos(int64_t val, uint32_t radix, bool isUpper, uint32_t *lengthPtr);
const char *utos(uint64_t val, uint32_t radix, bool isUpper, uint32_t *lengthPtr);

#endif // MEAL_PRINT_H