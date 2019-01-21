// RUN: %ucc -o %t %s
// RUN: %t
// RUN: %t | %output_check '4.5'

main()
{
	float a, b;

	a = 1.3f, b = 3.2f;

	printf("%.1f\n", a + b);

	return 0;
}
