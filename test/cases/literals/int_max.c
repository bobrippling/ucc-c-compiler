// RUN: %ucc -fsyntax-only %s

long a;
__typeof(-2147483648) a;

int b;
__typeof(-2147483647) b;

int c;
__typeof(2147483647) c;

long d;
__typeof(2147483648) d;
