// RUN: %check -e %s

f(extern int i); // CHECK: error: extern storage on "i"
