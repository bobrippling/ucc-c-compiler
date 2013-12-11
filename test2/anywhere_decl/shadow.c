// RUN: %check -e %s

main()
{
	int i = 5;

	printf("%d\n", i);

	int i = 3; // CHECK: /error: redefinition of "i"/

	return i;
}
