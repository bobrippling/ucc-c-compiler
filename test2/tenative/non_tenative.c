// RUN: %check -e %s
static int x[]; // CHECK: note: previous definition
int x[5]; // CHECK: error: mismatching definitions of "x"
