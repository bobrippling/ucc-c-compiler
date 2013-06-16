// RUN: %check %s
static int x[]; // CHECK: !/warning/
int x[5]; // CHECK: !/warning/
