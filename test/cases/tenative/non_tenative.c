// RUN: %check -e %s
static int x[];
int x[5]; // CHECK: error: static redefinition as non-static
