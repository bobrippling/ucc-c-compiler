// RUN: %ucc %s -o %t
// RUN: %t | grep -F '5.0'

neg(float f, double d)
{
	f = -f;
	d = -d;
}

float neg_f(float f)
{
	return -f;
}

main()
{
	printf("%f\n", neg_f(5));
}
