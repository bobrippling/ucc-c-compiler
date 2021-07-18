// RUN: %ocheck 0 %s -std=c99
// RUN: %check --only --prefix=c89 %s -std=c89
// RUN: %check --only --prefix=c99 %s -std=c99

main() // CHECK-c89: warning: control reaches end of
{
#include "../ocheck-init.c"
}
