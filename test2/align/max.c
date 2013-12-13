// RUN: %ucc -fsyntax-only %s
x __attribute__((aligned));
y __attribute__((aligned(8)));

_Static_assert(__alignof__(x) == (sizeof(void *) == 8 ? 16 : 8), "max?");
_Static_assert(_Alignof(y) == 8, "max?");
