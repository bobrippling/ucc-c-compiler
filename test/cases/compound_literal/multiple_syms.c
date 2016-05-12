// RUN: %ucc %s

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));
f(char *s)
{
	printf("f(\"%s\")\n", s);
}

f(char *s);
g(char *p);

main()
{
	/* the "" and the compound literal must be in different symtables,
	 * so the compound-literal::gen code doesn't generate all decls
	 * (i.e. generate the "") again.
	 */

	f(   ""          );
	g(   &(char){1}  );
}
