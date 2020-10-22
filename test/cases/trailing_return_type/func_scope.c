// RUN: %ucc -fsyntax-only %s

auto f(int x) -> __typeof(x)
{
	return x;
}
