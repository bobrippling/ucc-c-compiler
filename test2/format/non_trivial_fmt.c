f(int a, int b)
{
	int printf(char *, ...) __attribute((format(printf, 1, 2)));
	printf(a ? "b=%d (a=%s)\n" : "b=%d\n", b, a);
}
