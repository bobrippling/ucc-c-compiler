// RUN: %ucc %s 2>&1 | %check %s

struct { int i; } x[] = {
	[10] = 3,
	[0 ... 5] = 2,
	[0] = { 1 },
	[3] = 5, // WARN: /error: can't replace/
};
// 1, 2, 2, 5, 2, 2
