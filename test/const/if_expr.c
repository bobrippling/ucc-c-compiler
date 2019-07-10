// RUN: %ucc -c -o %t %s

int f();
i = __builtin_constant_p(f()) ? f() : 5;
j = 0 ? f() : 2;
