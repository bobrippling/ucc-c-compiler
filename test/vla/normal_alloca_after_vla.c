// RUN: %ocheck 0 %s

void abort(void);

g()
{
	return 0;
}

main()
{
	int n = 5; // an unaligned value
	int vla[n];

	vla[0] = 1;
	vla[1] = 2;
	vla[2] = 3;
	vla[3] = 4;

	{
		// this alloca is done in order after the vla,
		// but is coalesced to before the VLA by the stack logic
		int x = 99;

		if(x != 99) abort();

		if(g())
			return 3; // scope-leave/restore vla, but doesn't pop the vla state

		int y = 35;
		if(y != 35) abort();

		// more scope leave code
		for(int i = 0; i < 10; i++)
			if(i == 5)
				break;

		if(vla[0] != 1) abort();
		if(vla[1] != 2) abort();
		if(vla[2] != 3) abort();
		if(vla[3] != 4) abort();
		if(sizeof vla != n * sizeof(int))
			abort();

		if(y != 35) abort();
		if(x != 99) abort();
	}

	if(vla[0] != 1) abort();
	if(vla[1] != 2) abort();
	if(vla[2] != 3) abort();
	if(vla[3] != 4) abort();
	if(sizeof vla != n * sizeof(int))
		abort();

	return 0;
}
