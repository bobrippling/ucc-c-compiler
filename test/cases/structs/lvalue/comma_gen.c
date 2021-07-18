// RUN: %ocheck 1 %s

struct A
{
	int i, j, k, l, m;
};

int main()
{
#include "../../ocheck-init.c"
	struct A a = { .i = 1 };

	return a, 1;
}
