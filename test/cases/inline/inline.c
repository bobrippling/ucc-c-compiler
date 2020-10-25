// RUN: %check %s -fshow-inlined -finline-functions -fno-semantic-interposition
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

inline int f(int i)
{
	return 3 + i;
}

char *g(int x)
{
	return "hello" + f(x); // CHECK: note: function inlined
}

int main()
{
	printf("%s\n", g(1)); // CHECK: note: function inlined

	return f(2); // CHECK: note: function inlined
}
