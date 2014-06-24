// RUN: %check -e %s

int; // CHECK: warning: declaration doesn't declare anything
// CHECK: ^!/error/

// anonymous functions with one argument

int (const **a); // CHECK: warning: declaration doesn't declare anything

int (static **a); // CHECK: error: static storage on "a"
