// RUN: %ocheck 9 %s
main()
{
	struct
	{
#define BF(x) unsigned b ## x : 1
		BF(1);
		BF(2);
		BF(3);
		BF(4);
	} a;

	*(char *)&a = 0; // needed to clear the upper 4 bits
	a.b1 = 1;
	a.b2 = 0;
	a.b3 = 0;
	a.b4 = 1;

	char i = *(char *)&a; // 9
	return i;
}
