// RUN: %ucc %s -o %t
// RUN: %t | %output_check '5 {1,2,3}, { {1,2,hi}, {2,0,(null)}, {5,6,yo} }'

main()
{
	const int i = 5;
	const int x[] = { 1, 2, 3 };
	const struct
	{
		int i, j;
		char *s;
	} y[] = {
		1, 2, { "hi" },
		{ 2 },
		{ 5, 6, "yo" }
	};

	printf("%d {%d,%d,%d}, { {%d,%d,%s}, {%d,%d,%s}, {%d,%d,%s} }\n",
			i, x[0], x[1], x[2],
			y[0].i, y[0].j, y[0].s,
			y[1].i, y[1].j, y[1].s,
			y[2].i, y[2].j, y[2].s);
}
