// RUN: %ucc -c %s
// RUN: %layout_check %s
int x[]; // assume 1

f(int x[]);
