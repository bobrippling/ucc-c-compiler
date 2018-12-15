// RUN: %ucc -c -o %t %s

i = __builtin_constant_p(f()) ? f() : 5;
j = 0 ? f() : 2;
