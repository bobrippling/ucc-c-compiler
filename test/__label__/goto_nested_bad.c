// RUN: %check -e %s

main()
{
	{
		__label__ x;
x: // CHECK: warning: unused label 'x'
		f();
	}
	{
		__label__ x;
x: // CHECK: warning: unused label 'x'
		g();
	}

	goto x; // CHECK: error: label 'x' undefined
}
