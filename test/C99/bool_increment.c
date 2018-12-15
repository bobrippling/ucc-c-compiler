// RUN: %ocheck 0 %s

_Bool b;

_Bool pinc() { return b++; }
_Bool pdec() { return b--; }
_Bool inc() { return ++b; }
_Bool dec() { return --b; }

assert(int cond)
{
	if(!cond)
		abort();
}

test_pinc()
{
	_Bool local;

	b = 0;
	local = pinc();
	assert(b == 1 && local == 0);

	b = 1;
	local = pinc();
	assert(b == 1 && local == 1);
}

test_pdec()
{
	_Bool local;

	b = 0;
	local = pdec();
	assert(b == 1 && local == 0);

	b = 1;
	local = pdec();
	assert(b == 0 && local == 1);
}

test_inc()
{
	_Bool local;

	b = 0;
	local = inc();
	assert(local == b);
	assert(b == 1);

	b = 1;
	local = inc();
	assert(local == b);
	assert(b == 1);
}

test_dec()
{
	_Bool local;

	b = 0;
	local = dec();
	assert(local == b);
	assert(b == 1);

	b = 1;
	local = dec();
	assert(local == b);
	assert(b == 0);
}

main()
{
	test_pinc();
	test_pdec();
	test_inc();
	test_dec();

	return 0;
}
