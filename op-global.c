int glob, glob2;

// unary
int zero = !&glob;
int one = !!&glob;

//int sum1 = +&glob;
//int sum2 = -&glob;

// binary
int tru = &glob == &glob;
int fals = &glob != &glob;

int tru2 = &glob != &glob2;
int fals2 = &glob == &glob2;

int tru3 = &glob && &glob;
int tru4 = &glob || &glob;

int *p = &tru3 + 2;
//int *q = tru3 + 2; TODO

//int ifp = !1.0; TODO

//int addr_cmp1 = 1 == &glob; TODO
//int addr_cmp2 = 1 > &glob; TODO

//__attribute__((weak)) TODO
//void weak(); TODO
//int x = !weak;

#define assert(x) yo(#x, x)
yo(char *name, int x)
{
	if(!x)
		printf("%s bad\n", name);
}

main()
{
	assert(!zero);
	assert(!fals);
	assert(!fals2);

	assert(one);
	assert(tru);
	assert(tru2);
	assert(tru3);
	assert(tru4);
}
