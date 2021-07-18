// RUN: %ucc -emit=dump %s | grep -F "int (*const)[const 2]"

void f(int a[static const 3][const 2]);
