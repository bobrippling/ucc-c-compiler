// RUN: %check -e %s
int b[2.3]; // CHECK: /error: not an integral array size/
