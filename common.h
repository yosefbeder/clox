#ifndef clox_common_h
#define clox_common_h

// #define DEBUG_GC
// #define DEBUG_BYTECODE
// #define STRESS_TEST_GC

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum
{
    RESULT_COMPILE_ERROR,
    RESULT_RUNTIME_ERROR,
    RESULT_SUCCESS,
} Result;

#endif