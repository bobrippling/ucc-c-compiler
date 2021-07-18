// RUN: %ucc -fsyntax-only %s

main()
{
	int f();
	int f();
	int f();
}
