// RUN: %check -e %s

int f(int);

int i = 1 + f(3) * 2; // CHECK: error: global scalar initialiser not constant
// CHECK: ^ note: first non-constant expression here (funcall)
