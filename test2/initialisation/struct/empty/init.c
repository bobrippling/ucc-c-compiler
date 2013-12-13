// RUN: %layout_check %s
// RUN: %check %s

struct A
{
}; // CHEKC: /warning: empty struct/

struct Containter
{
	struct A a;
};

struct Pre
{
	int i;
	struct A a;
	int j;
};

struct Pre p = { 1, /* warn */ 2 }; // CHECK: /warning: missing {} initialiser for empty struct/
struct Pre q = { 1, {}, 2 };

main()
{
	struct A a = { 5 }; // CHECK: /warning: missing {} initialiser for empty struct/
	struct A b = {};

	struct Containter c = {{}};

	c.a = (struct A){};
}
