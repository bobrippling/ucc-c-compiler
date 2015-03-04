// RUN: %ucc -Xprint %s | grep -F 'int (*const a)[const 2]'

void f(int a[static const 3][const 2]);
