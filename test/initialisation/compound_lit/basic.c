// RUN: %ocheck 2 %s

main()
{
	int *p = (int [2]){
		1, 2,
	};

	return p[1];
}
