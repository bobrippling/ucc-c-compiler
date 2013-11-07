// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'sts[0] = { 0, 0, 0 }' 'sts[1] = { 0, 0, 0 }' 'sts[2] = { 0, 0, 0 }' 'sts[3] = { 0, 0, 0 }' 'sts[4] = { 0, 0, 0 }' 'sts[5] = { 0, 1, 0 }'

main()
{
	struct { int i, j, k; } sts[] = {
		[5] = { .j = 1 }
	};

	for(int i = 0; i < 6; i++)
		printf("sts[%d] = { %d, %d, %d }\n",
				i,
				sts[i].i, sts[i].j, sts[i].k);
}
