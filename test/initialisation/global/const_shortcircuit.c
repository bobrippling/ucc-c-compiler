// RUN: %ucc -fsyntax-only %s

int f(), q(void);

i = 1 || f();
//j = 0 || q();

//a = 1 && f();
b = 0 && q();
