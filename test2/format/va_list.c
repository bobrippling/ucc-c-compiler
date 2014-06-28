// RUN: %check %s

typedef __builtin_va_list va_list;

int vprintf(const char *, va_list) // CHECK: !/warn/
	__attribute((format(printf, 1, /*no va:*/0))); // CHECK: !/warn/
