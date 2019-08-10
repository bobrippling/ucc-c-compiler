// RUN: %check -e %s

main()
{
	__label__ x;
	x:
	{
		__label__ x; // CHECK: error: label 'x' undefined
	}
}
