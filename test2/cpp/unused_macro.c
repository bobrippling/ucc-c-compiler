// RUN: %check %s -E -Wunused-macros
// RUN: NWARNS=$(%ucc -E %s -Wunused-macros 2>&1 >/dev/null | grep -c 'unused macro'); test $NWARNS = 1

#include "unused_macro.def"

#define HELLO(x) x+1
// CHECK: warning: unused macro "HELLO"

main()
{
}
