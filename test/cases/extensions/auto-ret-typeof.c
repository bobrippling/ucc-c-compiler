// RUN: %ocheck 10 %s

auto add(int a, int b) -> __typeof(a + b)
{
	return a + b;
}

auto main() -> int
{
#include "../ocheck-init.c"
	return add(2 + 3, 5);
}
