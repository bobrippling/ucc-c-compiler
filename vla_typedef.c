f(int x)
{
	typedef short vla[x];

	x = 2000;

	vla a, b;

	return sizeof a + sizeof b;
}

main()
{
	return f(2); // 4 * 2
}
