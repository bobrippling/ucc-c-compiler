int a(const char *);
int a(      char *);

main()
{
	char *s = "hi";
	const char *s2 = "yo";

	a("hi");
	a(s);
	a(s2);
}
