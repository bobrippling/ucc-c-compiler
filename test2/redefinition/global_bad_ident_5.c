// RUN: %check -e %s

static int x; // CHECK: note: previous definition
int x; // CHECK: error: non-static redefinition of static "x"
