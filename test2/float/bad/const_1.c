// RUN: %check -e %s
int b[2.3]; // CHECK: error: array type isn't integral (double)
