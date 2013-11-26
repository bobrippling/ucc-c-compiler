// RUN: %check -e %s

main()
{
	char x[] = __func__; // CHECK: /error:.*must be initialised with an initialiser list/
}
