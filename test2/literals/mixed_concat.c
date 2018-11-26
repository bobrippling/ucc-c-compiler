// RUN: %ocheck 0 %s

int a[] = "abc" L"xyz";

extern void abort(void);

void assert(int x)
{
	if(!x)
		abort();
}

int main()
{
	assert(a[0] == 'a');
	assert(a[1] == 'b');
	assert(a[2] == 'c');
	assert(a[3] == 'x');
	assert(a[4] == 'y');
	assert(a[5] == 'z');
	assert(a[6] == '\0');

	return 0;
}
