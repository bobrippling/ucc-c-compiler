// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 4 ]

main()
{
	int *p = (int[][2]){
		1, 2,
		{ 3, 4, 5 }
	};

	return p[1][1]; // 4
}
