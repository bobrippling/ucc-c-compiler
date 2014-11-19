// RUN: %check -e %s

int k = 5; // CHECK: note: previous definition
// CHECK: ^ note: other definition here
static int k = 2; // CHECK: error: static redefinition of non-static "k"
// CHECK: ^ error: multiple definitions of "k"
