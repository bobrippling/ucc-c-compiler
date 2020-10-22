// RUN: %check -e %s
g(int i = 3); // CHECK: /error: parameter 'i' is initialised/
