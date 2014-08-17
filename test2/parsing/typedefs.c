// RUN: %ucc -fsyntax-only %s

typedef signed int t;
typedef int plain;
struct tag {
	unsigned t:4;
	const t:5;
	plain r:5;
};

t f(t (t));

g()
{
	long t;
}
