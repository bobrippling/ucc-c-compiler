// RUN: %check %s
extern a();
int x = 0 && a(); // CHECK: !/warn/
