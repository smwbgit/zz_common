#pragma once

#include <stdarg.h>

#define utils_va_start(format) va_list va; va_start(va, format)
#define utils_va_end() va_end(va)