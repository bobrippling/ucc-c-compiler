// RUN: %check --only %s -Wno-int-ptr-conversion

void *ptr(int a)
{
  return 3; // CHECK: /warning: mismatching types/
}
