// RUN: %check %s

int x __attribute((abc)); // CHECK: warning: ignoring unrecognised attribute "abc"
int y __attribute((abc)); // CHECK: !/warn/
