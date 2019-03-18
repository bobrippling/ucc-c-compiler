// RUN: %ucc -fsyntax-only %s

g(){ return 5; }
h(){ return 6; }

f(a, b)
{
	return a + b;
}

main()
{
	return f(g() + h());
}
