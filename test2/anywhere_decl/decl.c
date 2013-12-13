// RUN: %ucc -o %t %s && %t

main()
{
	int i;
	i = 5;

	int j;
	j = 2;

	int k;
	k = 3;

	// 10

	for(int i = 0; i < 3; i++){
		i++;

		int j = 3;
	}


	int l;
	l = 3;

	return (l + i + j + k) == 13 ? 0 : 1;
}
