// RUN: %ocheck 2 %s
f(){}

main()
{
	int j;

	f();
	j = 2;

	int i = j;
	return i;
}
