// RUN: %ocheck 0 %s

main()
{
#include "../ocheck-init.c"
  int a[4] = { 0 };
  return 3[a];
}
