// RUN: %check %s

_Bool f(float a)
{
	if(a == 5) // CHECK: !/warning/
		;

	return 0;
}
