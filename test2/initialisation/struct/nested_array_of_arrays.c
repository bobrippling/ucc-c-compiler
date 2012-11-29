// RUN: %ucc -c %s
// RUN: [ `%ucc %s -S -o- | grep 'mov.*[123456789]' | wc -l` -eq 9 ]
// RUN: [ `%ucc %s -S -o- | grep 'mov.*1[012]' | wc -l` -eq 3 ]

main()
{
	struct { int i, j, k; } a[][2] = {
		{ { 1, 2, 3 }, { 4, 5, 6 } },
		{ { 7, 8, 9 }, { 10, 11, 12 } },
	};
}
