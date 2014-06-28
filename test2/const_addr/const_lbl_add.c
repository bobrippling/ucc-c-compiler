// RUN: %layout_check %s

int a;
long x = (long)&a + 1;
long y = (long)"hi" + 2;
