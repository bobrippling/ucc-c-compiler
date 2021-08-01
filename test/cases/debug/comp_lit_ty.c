// RUN: %debug_check %s

main()
{
	return (struct A { int i, j; }){ .i = 5 }.j;
}
