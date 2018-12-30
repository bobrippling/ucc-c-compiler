// RUN: %ucc -fsyntax-only %s

extern void v;

main()
{
	printf("%p\n", &v);
}
