// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 6 ]

int i = (int){1};
int j[] = (int[]){ 1, 2 };

main()
{
	return i + j[1] + j[0] + *(int *){({static int x = 2; &x; })};
}
