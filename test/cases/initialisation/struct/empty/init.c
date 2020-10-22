// RUN: %layout_check %s
// RUN: %check %s

struct A
{
}; // CHECK: /warning: struct is empty/

struct Container
{
	struct A a;
};

struct Pre
{
	int i;
	struct A a;
	int j;
};

struct Pre p = { 1, /* warn */ 2 }; // CHECK: warning: missing {} initialiser for empty struct
struct Pre q = { 1, {}, 2 };

main()
{
	struct A a = { 5 }; // CHECK: warning: missing {} initialiser for empty struct
	struct A b = {};

	struct Container c = {{}};

	c.a = (struct A){};
}
