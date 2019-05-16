// RUN: %ocheck 4 %s -std=c99
// RUN: %ocheck 8 %s -std=c90
// RUN: %ocheck 8 %s -std=c89

struct A { int i; };

main()
{
	if((struct A { int i, j; } *)0)
		;

	return sizeof(struct A);
}
