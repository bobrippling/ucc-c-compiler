// RUN: %check -e %s

extern int f(int x[-1]); // CHECK: /error: negative array size/
