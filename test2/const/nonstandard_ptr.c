// RUN: %check %s -pedantic

#define NULL (void *)0

int y[1 + (int)NULL]; // CHECK: /warning:.*non-standard constant expression/
