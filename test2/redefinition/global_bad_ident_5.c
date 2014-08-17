// RUN: %check -e %s

static int x; // CHECK: note: previous definition
int x; // CHECK: error: mismatching definitions of "x"
