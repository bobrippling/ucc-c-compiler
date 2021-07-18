// RUN: %ucc -E %s >%t 2>&1
// RUN: grep 'in file included from: .*warning\.c:6' %t
// RUN: grep 'from: .*warning\.h:1' %t
// RUN: grep 'warning_\.h:2:1: warning: #warning: hello' %t

#include "warning.h"

main()
{
}
