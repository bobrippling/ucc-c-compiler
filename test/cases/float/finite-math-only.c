// RUN: %ocheck 1 %s -ffinite-math-only
// RUN: %ocheck 0 %s -fno-finite-math-only

int main()
{
#include "../ocheck-init.c"
	float a = __builtin_nanf("");

	return a == a;
}
