// RUN: %ucc -S -o- %s | grep 'x.*-12'
// RUN: %ucc -c %s

int x;
int *p = &x - 3;
