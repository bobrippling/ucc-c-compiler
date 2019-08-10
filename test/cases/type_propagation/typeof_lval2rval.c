// RUN: %ucc -fsyntax-only %s

volatile int *volatile p;

_Static_assert(__builtin_types_compatible_p(__typeof(*(p)), volatile int), "");
_Static_assert(__builtin_types_compatible_p(__typeof( (__typeof(*(p))) *(p) ), int), "");
