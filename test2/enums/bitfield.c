// RUN: %check %s

struct
{
	// implicitly unsigned
	enum
	{
		A = 8, // CHECK: warning: enumerator A (8) too large for its type (x)
		B = 2  // CHECK: !/warn/
	} x : 3;
	// 3 bits can represent 0-7
	// - should warn for A


	// implicitly signed
	enum
	{
		X = -5, // CHECK: warning: enumerator X (-5) too large for its type (y)
		Y = 3   // CHECK: !/warn/
	} y : 3;
	// 3 bits can represent -4-3
	// - should warn for X
} a;
