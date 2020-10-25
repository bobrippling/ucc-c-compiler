// RUN: %check -e %s
f()
{
	int x[2];
	return _Generic(x, int *: 1, int []: 2); // CHECK: error: incomplete type 'int[]' in _Generic
}
