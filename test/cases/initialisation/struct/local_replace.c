// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'x[0] = 3' 'x[1] = 3' 'x[2] = 3' 'x[3] = 2' 'x[4] = 3' 'x[5] = 3' 'x[6] = 3' 'x[7] = 3' 'x[8] = 3' 'x[9] = 3'
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	int x[] = { [0 ... 9] = 3, [3] = 2 };

	for(int i = 0; i < 10; i++)
		printf("x[%d] = %d\n", i, i[x]);

	return 5;
}
