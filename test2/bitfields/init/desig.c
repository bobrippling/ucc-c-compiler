// RUN: %ucc -o %t %s
// RUN: %t | %output_check '65536'

struct Desig
{
	int x : 16, y : 2;
} des = {
	.y = 1
};

main()
{
	printf("%d\n", *(int *)&des);
}
