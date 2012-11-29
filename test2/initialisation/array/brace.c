// RUN: %ucc %s
// RUN: [ `%ucc %s -S -o- | grep 'mov.*[1234]' | wc -l` -eq 4 ]

main()
{
	int a[][2] = {
		{ 1, 2,},
		{ 3, 4 },
	};
}
