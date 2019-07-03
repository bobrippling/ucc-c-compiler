// RUN: %layout_check %s

int *x = &(int){2};
int *y = (int[]){2, 3};
int *z = &(int[]){2, 3}[1];

int *s = "hello";
int *p = "hello" + 2;
