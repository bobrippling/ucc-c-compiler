// RUN: %check -e %s
int x[1, 2]; // CHECK: /error: expecting token '\]'/
