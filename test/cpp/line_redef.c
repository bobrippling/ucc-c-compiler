// RUN: %ucc -fsyntax-only %s

#line 1000

_Static_assert(__LINE__ == 1001, "");
