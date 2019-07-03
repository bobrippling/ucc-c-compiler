// RUN: %ucc -fsyntax-only %s

main()
{
	char (*p)[] = &"hello";
}
