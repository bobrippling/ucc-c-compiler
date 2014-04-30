// RUN: %check %s

void f() __attribute((warn_unused_result)); // CHECK: warning: warn_unused attribute on function returning void
