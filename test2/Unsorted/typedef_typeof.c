main()
{
	int i = 1;
	__typeof(int) i2 = 2;
	__typeof(i) i3 = 3;
	__typeof(*(int *)0) i4 = 4;
	__typeof(i4) i5 = 5;
	__typeof(i5) i6 = 6;

	typedef int int_t;
	int_t a = 7;
	typedef int_t int_t2;
	int_t2 b = 8;
	typedef int_t2 int_t3;
	int_t3 c = 9;

	__typeof(int_t3) d = 10;

	assert(i + i2 + i3 + i4 + i5 + i6 + a + b + c + d == 55);
}
