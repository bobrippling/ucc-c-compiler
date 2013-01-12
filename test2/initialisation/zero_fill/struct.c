// RUN: [ `%ucc %s -S -o- | grep 'mov.*0' | wc -l` -eq 1 ]
// RUN: [ `%ucc %s -S -o- | grep 'mov.*[12]' | wc -l` -eq 4 ]
// RUN: %ucc %s -S -o- | grep 'mov.*[^012]'; [ $? -ne 0 ]

main()
{
	struct
	{
		int a, b, c;
	} x = { 1, 2 /* 3 */ };
}

f()
{
	struct A
	{
		int i, j;
	} a = {
	};

	struct A b = { 1, 2, 3 };
}
