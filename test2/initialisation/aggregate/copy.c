// RUN: %ucc -o %t %s
// RUN: %t | %output_check '^x\[0\] = { 0, 0 }$' '^x\[1\] = { 1, 2 }$' '^x\[2\] = { 1, 2 }$' '^x\[3\] = { 1, 2 }$' '^x\[4\] = { 1, 2 }$' '^x\[5\] = { 1, 2 }$'

main()
{
	struct { int i, j; } x[] = {
		[1 ... 5] = { 1, 2 } // a memcpy is performed for these structs
	};

	for(int i = 0; i < 6; i++)
		printf("x[%d] = { %d, %d }\n",
				i, x[i].i, x[i].j);
}
