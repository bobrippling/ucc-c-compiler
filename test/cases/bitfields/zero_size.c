// RUN: %check %s
// RUN: %layout_check %s

struct A
{
	int : 0;
};

struct A a; // CHECK: !/warn/
struct A b = {}; // CHECK: !/warn/
struct A c = { 1 }; // CHECK: warning: excess initialiser for 'struct A'
struct A *d = &c; // CHECK: !/warn/
struct A e[] = { {}, {}, {} }; // CHECK: !/warn/

// -----------------------

struct B
{
	int : 10;
	int : 0;
	int a;
	int : 0;
	int b;
} f = { 6, 3 };

_Static_assert(
		sizeof(struct B) == 3 * sizeof(int),
		"int:0 member shouldn't affect size");
_Static_assert(_Alignof(f) == _Alignof(int), "");

// -----------------------

struct E1
{
  char x;
	char : 0;
  char y;
};

struct E2
{
  char x;
	int : 0;
  char y;
};

struct E1 e1 = { 1, 2 };
_Static_assert(sizeof(e1) == sizeof(char) * 2, "");
_Static_assert(_Alignof(e1) == 1, "");

struct E2 e2 = { 5, 9 };
_Static_assert(sizeof(e2) == 5, "");
_Static_assert(_Alignof(e2) == 1, "");
