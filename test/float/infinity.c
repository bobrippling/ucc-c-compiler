// RUN: %ucc -o %t %s
// RUN: %t | %output_check 'inf'
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

float inf = 1 / 0.0f;

main()
{
	printf("%f\n", inf);
}
