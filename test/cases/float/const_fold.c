// RUN: %ucc -o %t %s
// RUN: %t | grep '^-1.0$'

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

float f = -1.0f;

main()
{
	printf("%.1f\n", f);
}
