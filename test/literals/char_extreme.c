// RUN: %ocheck 0 %s

// check sign extension
_Static_assert('\xff' == -1, "");
_Static_assert((char)'\xff' == -1, "");

_Static_assert('abc'   == 0x616263, "");
_Static_assert('abcde' ==   0x62636465, "");

char x[] = "\xff";

void assert(int x)
{
	if(!x){
		extern void abort(void);
		abort();
	}
}

int main()
{
	assert(x[0] == -1);
	return 0;
}
