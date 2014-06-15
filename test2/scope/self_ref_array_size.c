// RUN: %check -e %s

h(int x[sizeof x]); // CHECK: error: undeclared identifier "x"
