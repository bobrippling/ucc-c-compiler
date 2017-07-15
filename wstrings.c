extern int printf(const char *,...);

int main()
{
	const char *msg = _Generic(
			"foo",
			const char *: "const char*",
			char*: "char *",
			char[4]: "hi",
			default: "something else");

	printf ("type \"foo\" = %s\n", msg);

	__typeof("123") cc = "456";
	cc[1] = 'x';
	printf("modified __typeof(\"...\") variable: %s\n", cc);

	return 0;
}
