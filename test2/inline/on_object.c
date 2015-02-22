// RUN: %check %s

volatile struct A
{
  int abc;
}; // CHECK: warning: ignoring volatile on no-instance struct

static struct B
{
  int abc;
}; // CHECK: warning: ignoring static on no-instance struct

inline int i; // CHECK: warning: inline on non-function
