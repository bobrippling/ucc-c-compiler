// RUN: %check -e %s

extern char x[32]; // CHECK: !/error/
char x[32]; // CHECK: !/error/

extern char y[32]; // CHECK: /note: previous definition/
char y[16]; // CHECK: /error: mismatching definitions of "y"/
