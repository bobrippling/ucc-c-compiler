// RUN: %check -e %s

typedef int size_t;

typedef size_t signed ssize_t; // CHECK: error: typedef instance can't be signed or unsigned
typedef signed size_t ssize_t; // CHECK: error: unknown type name 'size_t'
