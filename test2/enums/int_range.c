// RUN: %check %s -pedantic

enum E
{
	A = 1 << 31, // CHECK: warning: enumerator value 2147483648 out of 'int' range
	B = 1 << 32, // CHECK: warning: shift count >= width of int (32)
	C = 1l << 43 // CHECK: warning: enumerator value 8796093022208 out of 'int' range
};
