// RUN: %check -e %s

f(int ar[10][static 3]) // CHECK: error: static in non outermost array type
{
	return 0;
}
