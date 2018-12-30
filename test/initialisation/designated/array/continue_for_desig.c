// RUN: %ocheck 1 %s

int x[3] = {
	[2] = 5,
	[0] = 1 // don't exit the array init here
};

main()
{
	return x[0] == 1 && x[1] == 0 && x[2] == 5;
}
