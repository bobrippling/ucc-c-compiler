// RUN: %ocheck 0 %s

enum
{
	INT_MAX = -1u / 2 - 1
};

// this is UB, but defined in ucc as 2's complement wrapping
int under = -INT_MAX - 2;
int over = INT_MAX + 1;

main()
{
	if(under != 2147483647)
		abort();
	if(over != -2147483648)
		abort();

	return 0;
}
