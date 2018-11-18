// RUN: %check %s -fshow-inlined -finline-functions -fno-semantic-interposition

// inline
int f(int x)
{
	return x + 1;
}

int h(int);

// don't inline
int g(int x)
{
	int t = x;
	for(int i = 0; i < 10; i++)
		t += h(i);

	t++;

	return t * x;
}

int main()
{
	printf("%d\n", f(5)); // CHECK: note: function inlined
	printf("%d\n", g(5)); // CHECK: !/function inlined/
}
