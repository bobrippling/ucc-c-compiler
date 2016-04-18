// RUN: %ucc -o %t %s
// RUN: %t | %output_check yo
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));
int x()
{
	printf("yo\n");
}

int (*f(void))()
{
	return x;
}

main()
{
	f()();
	return 0;
}
