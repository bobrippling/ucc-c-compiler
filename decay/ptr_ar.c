int main()
{
	int x[] = { 4, 2 };
	int (*p)[sizeof x / sizeof *x] = &x;

	return **p;
}
