// RUN: %check %s

typedef int fn(void);
const fn x; // CHECK: warning: qualifier on function type 'fn {aka 'int (void)'}'
