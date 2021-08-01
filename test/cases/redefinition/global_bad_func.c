// RUN: %check -e %s

int f(void);

int f(int); // CHECK: /error: mismatching definitions of "f"/
