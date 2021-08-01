// RUN: %ucc -o %t %s
// RUN: %t | grep '^5.2 -5.2 5.2$'

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

float f1 = 5.2f;
float f2 = -5.2f;
double d = 5.2;

main()
{
	printf("%.1f %.1f %.1f\n", f1, f2, d);
}
