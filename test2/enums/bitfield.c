// RUN: %ucc -fsyntax-only %s

#define SIGNED(mem, t)             \
_Static_assert(                    \
		__builtin_is_signed(mem) == t, \
		#mem " signed != " #t);

struct
{
	// implicitly unsigned
	enum { A = 8, B = 2 } x : 3; // CHECK: /enum member A too large for type/
	// 3 bits can represent 0-7
	// - should warn for A


	// implicitly signed
	enum { X = -5, Y = 3 } y : 3; // CHECK: /enum member X too large for type/
	// 3 bits can represent -4-3
	// - should warn for X
} a;

SIGNED(a.x, 0);
SIGNED(a.y, 1);
