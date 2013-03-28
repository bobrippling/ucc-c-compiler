struct A
{
};

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

struct Pre p = { 1, /* warn */ 2 };
struct Pre q = { 1, {}, 2 };

main()
{
	struct A a = { 5 };
	struct A b = {};

	struct Containter c = {{}};

	c.a = (struct A){};
}
