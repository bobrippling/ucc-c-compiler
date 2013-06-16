// RUN: %check -e %s

int f(void); // CHECK: /error: mismatching definitions of "f"/

int f(int);
