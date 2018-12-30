// RUN: %layout_check %s
int **p = (int *[]) { (int[]) {1,2,3}, (int[]) {4,5,6} };
