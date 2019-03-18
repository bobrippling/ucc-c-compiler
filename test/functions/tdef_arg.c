// RUN: %ucc -fsyntax-only %s

typedef int int_t;

f(int);

f(int_t i)
{
	return i;
}
