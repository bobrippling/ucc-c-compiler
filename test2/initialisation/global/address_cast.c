// RUN: %check -e %s

// this is a bad constant cast
int x;
int *p = (int)&x; // CHECK: /global scalar initialiser not constant/
