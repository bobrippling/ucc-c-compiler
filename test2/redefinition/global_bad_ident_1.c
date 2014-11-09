// RUN: %check -e %s

static int j = 2; // CHECK: note: previous definition
int j; // CHECK: error: non-static redefinition of static "j"
