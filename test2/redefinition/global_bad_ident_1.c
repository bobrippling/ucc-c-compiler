// RUN: %check -e %s

static int j = 2; // CHECK: note: previous definition
int j; // CHECK: error: mismatching definitions of "j"
