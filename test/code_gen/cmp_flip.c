// RUN: %ocheck 0 %s

static int is_ge_0(int c)
{
	return '0' <= c;
}

static int isodigit(int c)
{
	return '0' <= c && c <= '8';
}

static void assert(int b)
{
	if(b)
		return;
	_Noreturn void exit(int);
	exit(1);
}

int main()
{
	assert(!is_ge_0('0'-1));
	assert(is_ge_0('0'));
	assert(is_ge_0('1'));
	assert(is_ge_0('9'));

	assert(!isodigit('0'-1));
	assert(isodigit('0'));
	assert(isodigit('1'));
	assert(isodigit('8'));
	assert(!isodigit('9'));
}
