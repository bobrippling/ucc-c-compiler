// RUN: %ucc -fsyntax-only %s

main()
{
	extern int i;
	extern int i;
}
