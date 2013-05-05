// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'x\[0\] = { 1, 2, 3 }' 'x\[1\] = { 7, 5, 6 }' 'x\[2\] = { 7, 5, 6 }' 'x\[3\] = { 7, 5, 6 }'

f(){ return 7; }

main()
{
	struct A
	{
		int i, j, k;
	} x[] = {
		{ 1, 2, 3 },
		{ 1, 2, 3 },
		[1 ... 3] = { f(), 5, 6 }
	};

	for(int i = 0; i <= 3; i++)
		printf("x[%d] = { %d, %d, %d }\n",
				i,
				x[i].i,
				x[i].j,
				x[i].k);
}
