// RUN: %check -e %s

int k = 5; // CHECK: note: previous definition
static int k = 2; // CHECK: error: mismatching definitions of "k"
