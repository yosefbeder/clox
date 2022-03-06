#ifndef clox_error_h
#define clox_error_h

#include "scanner.h"
#include "vm.h"

typedef enum {
    REPORT_SCAN_ERROR,
    REPORT_PARSE_ERROR,
    REPORT_RUNTIME_ERROR,
    REPORT_WARNING,
} ReportType;

void report(ReportType, Token *, char[], struct Vm*);

#endif