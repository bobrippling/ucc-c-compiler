// RUN: %check -e %s
int i;
int i;
int i = 2;
int i = 3; // CHECK: /error: multiple definitions of "i"/
int i;
