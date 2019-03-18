// RUN: %ucc -fsyntax-only %s

main()
{
	(struct { int i, j, k; }){.k = 3}.i = 3;
}
