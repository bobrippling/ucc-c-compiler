// RUN: %ucc -fsyntax-only %s

main()
{
	sizeof(int[]){1};
}
