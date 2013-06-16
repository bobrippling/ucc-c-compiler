// RUN: %check %s
int x[]; // CHECK: !/warning/
int x[]; // CHECK: /warning: tenative array definition assumed to have one element/
