int f(int *ar, int n);
int f();

f(ar, n)
	int n;
	int ar[sizeof n];
{
	return ar[2];
}

/*
 * TODO: error test
h(i, j)
	int i[sizeof j]; // undeclared
{
	return i[j];
}
*/
