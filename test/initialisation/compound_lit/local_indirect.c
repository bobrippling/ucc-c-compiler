// RUN: %ocheck 7 %s

main()
{
	/* compound literal indirectly referenced in local scope wasn't code-gen'd,
	 * so p ends up not pointing to anything valid */
	static int *p = &(int){ 7 };

	return (int[]){ 3, *p, 5 }[1];
}
