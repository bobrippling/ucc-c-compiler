// RUN: %ucc -fsyntax-only %s

_Static_assert(_Generic(0L + 0LL, long long: 1) == 1, "");
