// RUN: %ucc -fsyntax-only %s

__typeof(1 == 2) f();
__typeof(1 && 2.0) f();
__typeof(1.0 == 2.0) f();

int f(int a, int b)
{
	return a == b;
}
