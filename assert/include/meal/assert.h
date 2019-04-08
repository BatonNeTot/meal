#ifndef MEAL_ASSERT_H
#define MEAL_ASSERT_H

#include "meal/log.h"

#ifndef NDEBUG

#define ASSERT_WARNING(expression, tag, message, ...) if (!(expression) &&\
log_warning(tag, "**WARNING!\n**File '%s'\n**Function '%s' in line %d\n"message, __FILE__, __func__, __LINE__, ##__VA_ARGS__))

#define ASSERT_ERROR(expression, tag, message, ...) if (!(expression) &&\
log_error(tag, "**ERROR!!\n**File '%s'\n**Function '%s' in line %d\n"message, __FILE__, __func__, __LINE__, ##__VA_ARGS__))

#define ASSERT_FATAL(expression, tag, message, ...) if (!(expression) &&\
log_fatal(tag, "**FATAL!!!\n**File '%s'\n**Function '%s' in line %d\n"message, __FILE__, __func__, __LINE__, ##__VA_ARGS__))

#define ASSERT_ELSE else

#else //NDEBUG

#define ASSERT_WARNING(expression, tag, message, ...) (void)(expression); while (0)

#define ASSERT_ERROR(expression, tag, message, ...) (void)(expression); while (0)

#define ASSERT_FATAL(expression, tag, message, ...) (void)(expression); while (0)

#define ASSERT_ELSE (void)0;

#endif //NDEBUG

#endif // MEAL_ASSERT_H