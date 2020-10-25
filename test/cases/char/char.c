// RUN: %ucc -fsyntax-only %s

#define TY_NEQ(TA, TB) \
	_Static_assert(!__builtin_types_compatible_p(TA, TB), "")

TY_NEQ(char,   signed char);
TY_NEQ(char, unsigned char);
TY_NEQ(signed char, unsigned char);
