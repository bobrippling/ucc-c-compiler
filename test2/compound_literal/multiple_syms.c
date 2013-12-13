// RUN: %ucc %s
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
