// RUN: %ucc -o %t %s
// RUN: %t | %stdoutcheck %s

// STDOUT: x[0] = 3
// STDOUT-NEXT: x[1] = 3
// STDOUT-NEXT: x[2] = 3
// STDOUT-NEXT: x[3] = 2
// STDOUT-NEXT: x[4] = 3
// STDOUT-NEXT: x[5] = 3
// STDOUT-NEXT: x[6] = 3
// STDOUT-NEXT: x[7] = 3
// STDOUT-NEXT: x[8] = 3
// STDOUT-NEXT: x[9] = 3

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	int x[] = { [0 ... 9] = 3, [3] = 2 };

	for(int i = 0; i < 10; i++)
		printf("x[%d] = %d\n", i, i[x]);

	return 5;
}
