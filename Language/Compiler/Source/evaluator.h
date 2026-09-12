/* Core macro and compile-time execution kernel. */
#ifndef RANGE_COMPILER_EVALUATOR_H
#define RANGE_COMPILER_EVALUATOR_H
#include "ast.h"
#include <stdint.h>

int rangeExecute(RangeArena *arena, RangeNode **units, size_t count,
                 const char *entry, int64_t *result, char *error, size_t errorSize);
#endif
