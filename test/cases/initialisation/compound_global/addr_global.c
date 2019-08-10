// RUN: %layout_check %s

int (*p)[] = &(int[]){1, 2, 3};
