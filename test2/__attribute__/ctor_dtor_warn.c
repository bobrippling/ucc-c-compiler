// RUN: %check %s

int x __attribute((constructor)); // CHECK: warning: constructor attribute on non-function
