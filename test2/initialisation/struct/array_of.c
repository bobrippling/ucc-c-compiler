// RUN: %ucc -c %s
// RUN: [ `%ucc %s -S -o- | grep 'mov.*[1234]' | wc -l` -eq 4 ]

main()
{
	struct
	{
		int i, j;
	} a[] = {
		{ 1, 2 },
		{ 3, 4 }
	};
}
