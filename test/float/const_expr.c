// RUN: %check %s -pedantic

double d = -0.0; // CHECK: !/warning:.*standard/
double e = 1.0 + 1.0; // CHECK: warning: global scalar initialiser contains non-standard constant expression
double f = 1.0 == 1.0; // CHECK: warning: global scalar initialiser contains non-standard constant expression
