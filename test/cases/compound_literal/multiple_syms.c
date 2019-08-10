// RUN: %ucc %s -o %t
// check this links

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));
f(char *s)
{
	printf("f(\"%s\")\n", s);
}

g(char *p)
{
	printf("g(%p) = %d\n", p, *p);
}

main()
{
	/* the "" and the compound literal must be in different symtables,
	 * so the compound-literal::gen code doesn't generate all decls
	 * (i.e. the "") again.
	 */

	f(   ""          );
	g(   &(char){1}  );
}
