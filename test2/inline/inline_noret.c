// RUN: %ucc -o %t %s

f(int ok)
{
	if(ok)
		return 1;

	return; // ensure this doesn't cause a crash
}

main()
{
	int x = f(1);
	int y = f(0);

	return x + y;
}
