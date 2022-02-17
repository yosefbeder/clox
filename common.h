#ifndef clox_common_h
#define clox_common_h

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
    RESULT_COMPILE_ERROR,
    RESULT_RUNTIME_ERROR,
    RESULT_SUCCESS,
} Result;

#endif