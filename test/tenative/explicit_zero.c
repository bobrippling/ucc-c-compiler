// RUN: %ucc -fsyntax-only %s

int ar[] = {};
_Static_assert(sizeof(ar) == 0, "");
