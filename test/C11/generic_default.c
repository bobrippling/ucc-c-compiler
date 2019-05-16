// RUN: %ucc -fsyntax-only %s

_Static_assert(_Generic('a', default: 2, char: 5) == 2, "");
