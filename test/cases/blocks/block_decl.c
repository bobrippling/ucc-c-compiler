// RUN: %ocheck 0 %s

total;

int dispatch(int(char));

int dispatch(int x(char))
{
	x('B');
}

pc(char c)
{
	total += c;
}

main()
{
	int (^x)(char) = ^int(char tim){ total += tim; return 0; };
	int (^(*y))(char) = &x;
	void (^z)(void) = 0;

	x('a');
	(*y)('a');
	(0 ? x : *y)('a');

	dispatch(pc);

	if(total != 357){
		_Noreturn void abort();
		abort();
	}

	return 0;
}
