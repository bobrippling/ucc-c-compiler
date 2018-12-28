// RUN: [ `%ucc %s -S -o- | grep 'mov.*[12]' | wc -l` -eq 2 ]

main()
{
	struct A
	{
		int i, j;
	} a = { 1, 2 };
}
