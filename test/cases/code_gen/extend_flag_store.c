// RUN: %ocheck 0 %s -DINT_TY=int
// RUN: %ocheck 0 %s -DINT_TY=long\ long

void *memset(void *, int, unsigned long);

f()
{
	return 3;
}

void abort(void);

main()
{
	volatile INT_TY i;

	// write non-zero bits to 'i'
	memset(&i, 0xff, sizeof i);

	// ensure we sign extend the V_FLAG/_Bool and store to all bits of 'i'
	i = !!f();

	if(i != 1)
		abort();

	return 0;
}
