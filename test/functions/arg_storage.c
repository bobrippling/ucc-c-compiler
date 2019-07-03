// RUN: %check -e %s

void x(int, auto); // CHECK: error: auto storage on unnamed argument

void x(int, auto y); // CHECK: error: auto storage on "y"
