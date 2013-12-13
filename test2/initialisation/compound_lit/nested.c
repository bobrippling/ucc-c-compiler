// RUN: %ocheck 4 %s

main()
{
	int (*p)[2] = (int[][2]){
		1, 2,
		{ 3, 4, 5 }
	};

	return p[1][1]; // 4
}
