// RUN: %ucc -fsyntax-only %s

_Static_assert(
		_Generic((char)0, const char: 5, default: 1) == 1
		, "char shouldn't be compatible with const char");

_Static_assert(
		_Generic((const char)0, const char: 5, default: 1) == 1
		, "const char should decay to char");

_Static_assert(
		_Generic((const char)0, const char: 5, char: 6, default: 1) == 6
		, "const char should decay to char and char+constchar should be accepted");
