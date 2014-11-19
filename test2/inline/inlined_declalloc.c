// RUN: %ocheck 12 %s

__attribute((always_inline))
f(int x)
{
	int counts[] = { x-1, x, x+1 };

	int t = 0;
	for(int i = 0; i < sizeof(counts)/sizeof(counts[0]); i++)
		t += counts[i];

	return t;
}

main()
{
	int stash[] = { 1, 2 };

	// 1 + 9 + 2
	return stash[0] + f(3) + stash[1];
}
