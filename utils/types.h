#ifndef TYPES_H
#define TYPES_H

#include "pstdint.h"
#include <float.h>

typedef uint32_t KEY_TYPE;
typedef uint64_t POINTER_TYPE;
typedef double VALUE_TYPE;

#define KEY_TYPE_MAX UINT32_MAX
#define VALUE_TYPE_MAX DBL_MAX

#define KEY_TYPE_PRINTF_MODIFIER PRINTF_INT32_MODIFIER
#define POINTER_TYPE_PRINTF_MODIFIER PRINTF_INT64_MODIFIER

#endif /* TYPES_H */
