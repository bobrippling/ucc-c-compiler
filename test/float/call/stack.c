// RUN: %ucc -o %t %s
// RUN: %t | %output_check '1.00 2.00 3.00 4.00 5.00 6.00 7.00 8.00 9.00'
// RUN: %ucc -o %t %s -fstack-protector-all
// RUN: %t | %output_check '1.00 2.00 3.00 4.00 5.00 6.00 7.00 8.00 9.00'
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

g(float a, float b, float c, float d,
  float e, float f, float g, float h,
  float i)
{
	printf("%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
			a, b, c, d, e, f, g, h, i);
}

main()
{
	g(1, 2, 3, 4, 5, 6, 7, 8, 9);
}
