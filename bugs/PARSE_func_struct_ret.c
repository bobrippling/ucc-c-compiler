struct { int j, i; } *f()
{
	__typeof(*f()) static x;
	x.i = 3;
	return &x;
}

main()
{
	return f()->i;
}
