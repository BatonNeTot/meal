#ifndef MEAL_MATH_H
#define MEAL_MATH_H

#include "meal/macros.h"

#define MAX(a, b) FTERN(a > b, a, b)

#define MIN(a, b) FTERN(a < b, a, b)

#define SIGN(x) (((x) > 0) - ((x) < 0))

#define IN_RANGE(value, left, right) (((right) <= (value)) - ((value) < (left)))

#define CMPR(value1, value2) (((value2) < (value1)) - ((value1) < (value2)))

#endif // MEAL_MATH_H