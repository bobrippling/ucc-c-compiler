// RUN: %ucc -fsyntax-only %s

#define UINT_MAX -1U

enum E
{
	X = UINT_MAX
};

_Static_assert(
		_Generic(X, unsigned: 1) == 1,
		"bad integer constant promotion");
