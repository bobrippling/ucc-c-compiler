// RUN: %ucc -fsyntax-only %s

f(int);

f(const int i)
{
	return i;
}

struct A
{
	int i;
};

g(struct A);

g(const struct A a)
{
}
