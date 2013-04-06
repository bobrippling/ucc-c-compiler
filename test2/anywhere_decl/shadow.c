// RUN: %ucc %s; [ $? -ne 0 ]
// RUN: %check %s

main()
{
	int i = 5;

	printf("%d\n", i);

	int i = 3; // CHECK: /error: "i" already declared/

	return i;
}
