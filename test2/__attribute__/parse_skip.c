// RUN: %ucc -fsyntax-only %s

int __attribute__((unknown(char, abc(16)) )) a1;
int __attribute__((yo(z, w "", aligned(16)) )) a2;
