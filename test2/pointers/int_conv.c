// RUN: %check %s

void *ptr(int a)
{
  return 3; // CHECK: /warning: mismatching types/
}
