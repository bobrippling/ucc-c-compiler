// RUN: %check -e %s
typedef unsigned long size_t;
typedef int size_t; // CHECK: /error: second type "size_t" specified after primitive/
