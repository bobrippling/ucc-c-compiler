// RUN: %ocheck 0 %s

f_calls;
f()
{
	f_calls++;
	return 7;
}

q_calls;
q()
{
	q_calls++;
	return 8;
}

main()
{
	char x[20] = {
		[4] = q(),
		[5 ... 9] = f(),
	};

	struct A { int i, j; } y[] = {
		[3 ... 4] = { 1, 2 }
	};

	if(f_calls != 1 || q_calls != 1)
		abort();

	for(int i = 0; i < 4; i++)
		if(x[i] != 0)
			abort();

	if(x[4] != 8) abort();
	if(x[5] != 7) abort();
	if(x[6] != 7) abort();
	if(x[7] != 7) abort();
	if(x[8] != 7) abort();
	if(x[9] != 7) abort();

	for(int i = 10; i < 20; i++)
		if(x[i] != 0)
			abort();

	if(y[0].i || y[0].j)
		abort();
	if(y[1].i || y[1].j)
		abort();
	if(y[2].i || y[2].j)
		abort();
	if(y[3].i != 1 || y[3].j != 2)
		abort();
	if(y[3].i != 1 || y[3].j != 2)
		abort();

	return 0;
}
