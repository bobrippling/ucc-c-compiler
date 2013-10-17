// RUN: %check -e %s
typedef unsigned long size_t; // CHECK: /error: mismatching definition/
typedef int *size_t;
