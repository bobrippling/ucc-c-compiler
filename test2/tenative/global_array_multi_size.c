// RUN: %check -e %s

int x[];
int x[2]; // CHECK: /error: mismatching definitions of "x"/
int x[3];
