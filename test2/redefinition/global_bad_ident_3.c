// RUN: %check -e %s
int p = 2; // CHECK: note: other definition here
int p = 2; // CHECK: error: multiple definitions of "p"
