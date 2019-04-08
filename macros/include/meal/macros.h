#ifndef MEAL_MACROS_H
#define MEAL_MACROS_H

#include "meal/platform.h"

#include <stdint.h>
#include <stdbool.h>

#define CONCAT2(a, b) a ## b
#define CONCAT(a, b) CONCAT2(a, b)

#define TO_STR2(x) #x
#define TO_STR(x) TO_STR2(x)

#define LOOP(iterator, count) for (uint32_t iterator = (count); iterator > 0; iterator--)

#define ITOB(x) ((x) != 0)

#define IF(expression, statement) ((0-(expression)) & (statement))

#define TERN(value, statement1, statement2) (IF(value, (statement1)) + IF(!(value), (statement2)))

#define TERNP(value, statement1, statement2) TERN(value, (WST)(statement1), (WST)(statement2))

#define FTERN(expression, statement1, value2) (IF(expression, (statement1) - (value2)) + (value2))

#define FTERNP(expression, statement1, value2) FTERN(expression, (WST)(statement1), (WST)(value2))

#endif //MEAL_MACROS_H