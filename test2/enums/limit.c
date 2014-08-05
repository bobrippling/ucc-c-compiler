// RUN: %check -e %s -pedantic

#define INT_MAX 2147483647

enum fine
{
	X = INT_MAX, // CHECK: !/warn|err/
	Y = INT_MAX - 1, // CHECK: !/warn|err/
	Z // CHECK: !/warn|err/
};

enum exceed
{
	A = INT_MAX,     // CHECK: !/warn|err/
	B = INT_MAX + 1, // CHECK: warning: enumerator value 2147483648 out of 'int' range
};

enum exceed_implicit
{
	C = INT_MAX, // CHECK: !/warn|err/
	D,           // CHECK: error: overflow for enum member exceed_implicit::D
};
