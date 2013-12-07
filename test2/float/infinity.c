// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'inf'

float inf = 1 / 0.0f;

main()
{
	printf("%f\n", inf);
}
