// RUN: %inline_check %s
// RUN: %ocheck 3 %s

static void f(int const *p)
{
	*(int *)p = 3;
}

int main()
{
	int i;
	f(&i);
	return i;
}
