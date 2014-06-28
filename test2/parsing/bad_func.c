// RUN: %check -e %s

int; // CHECK: warning: declaration doesn't declare anything
// CHECK: ^!/error/

// anonymous functions with one argument

int (const **a); // CHECK: error: unnamed function declaration

int (static **a); // CHECK: error: static storage on "a"
