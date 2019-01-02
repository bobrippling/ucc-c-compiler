// RUN: %check %s

int printf(int, ...)
	__attribute__((format(printf, 1, 2))); // CHECK: warning: format argument not a string type
