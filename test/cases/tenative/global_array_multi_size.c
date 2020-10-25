// RUN: %check -e %s

int x[];
int x[2];
int x[3]; // CHECK: /error: mismatching definitions of "x"/
