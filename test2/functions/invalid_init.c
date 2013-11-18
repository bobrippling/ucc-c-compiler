// RUN: %check -e %s
f() = 2; // CHECK: /error: initialisation of function 'f'/
