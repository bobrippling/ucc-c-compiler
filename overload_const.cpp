#define __overloadable __attribute__((overloadable))

int a(const char *) __overloadable; // Z1aPKc
int a(      char *) __overloadable; // Z1aPc


main()
{
	char *s = "hi";
	const char *s2 = "yo";

	a("hi");
	a(s);
	a(s2);
}
