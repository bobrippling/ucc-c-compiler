// RUN: %ucc -g -o %t -S %s
// RUN: grep 'sub1' %t >/dev/null
// RUN: grep 'sub2' %t >/dev/null
// RUN: grep 'sub3' %t >/dev/null
// RUN: grep 'asint' %t >/dev/null

typedef union
{
	struct
	{
		int sub1, sub2, sub3;
	};

	int asint;
} U;

main()
{
	U u = { 0 };
	u.sub2 = 3;
	return u.asint;
}
