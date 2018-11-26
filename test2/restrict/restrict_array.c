// RUN: %check %s

int f(int a[const restrict static 2]); // CHECK: !/warn/
