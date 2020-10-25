// RUN: %check -e %s
int b[2.3]; // CHECK: error: array size isn't integral (double)
