// RUN: %ucc -fsyntax-only %s

f(a, b, c, d)
	char a;
{
}

main()
{
	f(1); // used to crash here - oob expr_args
}
