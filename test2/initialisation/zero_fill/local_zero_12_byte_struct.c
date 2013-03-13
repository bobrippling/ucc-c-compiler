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
