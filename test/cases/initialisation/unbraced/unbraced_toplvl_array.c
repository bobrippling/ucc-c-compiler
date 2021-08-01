// RUN: %check -e %s

int x[] = 1; // CHECK: error: int[] must be initialised with an initialiser list
