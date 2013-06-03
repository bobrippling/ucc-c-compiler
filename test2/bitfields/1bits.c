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

	*(int *)&a = 0;
	a.b1 = 1;
	a.b2 = 0;
	a.b3 = 0;
	a.b4 = 1;

	int i = *(int *)&a; // 9
	return i;
}
