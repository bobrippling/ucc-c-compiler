// RUN: %ucc -o %t %s
// RUN: %t | %output_check '-1.0f'

float f = -1.0f;

main()
{
	printf("%.1f\n", f);
}
