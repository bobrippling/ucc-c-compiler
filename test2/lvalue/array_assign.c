// RUN: %check -e %s

f(int (*pa)[2])
{
	int x[2];
	*pa = x; // CHECK: error: cast to array type 'int[2]'
}
