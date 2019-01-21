// RUN: %check -e %s

f(int x[const 7]);

int a[const 5]; // CHECK: array has qualified type
