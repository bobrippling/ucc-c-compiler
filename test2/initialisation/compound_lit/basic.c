// RUN: %ucc %s -o %t
// RUN: %t; [ $? -eq 2 ]

main()
{
	int *p = (int [2]){
		1, 2,
	};

	return p[1];
}
