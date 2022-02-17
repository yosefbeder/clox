#ifndef clox_error_h
#define clox_error_h

#include "scanner.h"

typedef enum {
    ERROR_SCAN,
    ERROR_PARSE,
    ERROR_RUNTIME,
} ErrorType;

void reportError(ErrorType, Token*, char[]);

#endif