// RUN: %check -e %s
int h(int x[i], int i); // CHECK: error: undeclared identifier "i"
