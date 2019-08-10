// RUN: %ucc -fsyntax-only %s

_Static_assert(0 == (0 && "hello"), "");
_Static_assert(1 == (1 || "yo"),    "");
_Static_assert(1 == (1 && "hello"), "");
_Static_assert(1 == (0 || "yo"),    "");
