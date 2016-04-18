// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

done_f, done_g, done_h;

f(){ if(done_f) abort(); done_f = 1; return 3; }
g(){ if(done_g) abort(); done_g = 1; return 4; }
h(){ if(done_h) abort(); done_h = 1; return 3; }

test()
{
	short ar[f()][g()];

	// sizeof(ar) = 2 * 3 * 4 = 24
	// sizeof(ar[0]) = 4 * 2 = 8
	// + = 32
	return sizeof(ar) + sizeof(ar[0]);
}

test_nodecl()
{
	return sizeof(long[h()]);
}

main()
{
	int sz = test();

	if(sz != 32)
		abort();

	sz = test_nodecl();
	if(sz != 24)
		abort();

	return 0;
}
