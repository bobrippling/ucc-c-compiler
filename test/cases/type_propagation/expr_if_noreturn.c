// RUN: %ucc -fsyntax-only %s

_Noreturn void exit(int);
__attribute((noreturn)) void exit2(int);
void g(int i);

_Static_assert(
		!__builtin_has_attribute(g, noreturn),
		"");

_Static_assert(
		__builtin_has_attribute(exit, noreturn),
		"");

_Static_assert(
		__builtin_has_attribute(exit2, noreturn),
		"");

_Static_assert(
		!__builtin_has_attribute(
			(1 ? exit : g),
			noreturn),
		"");

_Static_assert(
		!__builtin_has_attribute(
			(1 ? exit2 : g),
			noreturn),
		"");
