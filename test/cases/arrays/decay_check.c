// RUN: %ucc -fsyntax-only %s

#define is_array(x) !__builtin_types_compatible_p(\
		__typeof(x), \
		__typeof(&(x)[0]))

int x[3];
int *p;

_Static_assert(is_array(x), "");
_Static_assert(!is_array(p), "");
