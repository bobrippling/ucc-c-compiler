// RUN: %check -e %s

#define NULL (void *)0

int a = -NULL; // CHECK: error: - requires an arithmetic type (not "void *")
int b = NULL+NULL; // CHECK: error: operation between two pointers must be relational or subtraction
int c = NULL+.0f; // CHECK: error: implicit cast from pointer to floating type
int d[NULL+1]; // CHECK: error: array size isn't integral (void *)

int e = 0?NULL:1;

int x = +NULL; // CHECK: error: + requires an arithmetic type (not "void *")
int y = ~NULL; // CHECK: error: ~ requires an integral type (not "void *")
int z = -NULL; // CHECK: error: - requires an arithmetic type (not "void *")
