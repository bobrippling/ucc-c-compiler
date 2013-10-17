// RUN: %check -e %s
typedef unsigned long size_t; // CHECK: mismatching definitions
typedef int size_t;
