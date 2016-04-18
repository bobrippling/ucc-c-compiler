// RUN: %ucc -fsyntax-only %s

_Static_assert(0 == !0.3, "");
_Static_assert(1 == !!__builtin_nan(""), "");
