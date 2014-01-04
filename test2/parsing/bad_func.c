// RUN: %check -e %s

int; // CHECK: !/error/

// anonymous functions with one argument

int (static **a); // CHECK: error: static storage on parameter
// CHECK: ^error: declaration doesn't declare anything

int (const **a); // CHECK: error: declaration doesn't declare anything
