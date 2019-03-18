// RUN: %ucc -o %t %s
// RUN: %ocheck 5 %t

int i;

int *x()
{
	return &i;
}

main()
{
	*x() = 5;

	return i;
}
