// RUN: %layout_check %s
typedef int *iptr;

iptr x[] = { 1, 2, 3 }; // need casts inserting
