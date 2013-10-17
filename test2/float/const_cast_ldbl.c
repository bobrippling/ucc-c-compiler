// RUN: %ucc -o %t %s
// RUN: %t | %output_check '-1.0' -10

float f = -1;
int i = -10.0f;

main()
{
	printf("%.1f\n", f);
	printf("%d\n", i);
}
