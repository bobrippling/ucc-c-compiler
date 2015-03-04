// RUN: %ucc -Xprint %s | grep -F 'int a[static const 3][const 2]'

int a[static const 3][const 2];
