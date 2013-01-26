// RUN: %ucc -o %t %s && %t

f()
{
	_Static_assert(__builtin_constant_p((char)5678), "cast not a constant");
	return (char)5678;
}

main()
{
	return f() == 46 ? 0 : 1;
}
