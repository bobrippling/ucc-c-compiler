// RUN: %ucc -fsyntax-only %s

f() __attribute(());

f(i)
	__attribute__(()) int i;
{
	return i + 1;
}

g() __attribute((yo_yo))
{
}

main()
{
	return f(2);
}
