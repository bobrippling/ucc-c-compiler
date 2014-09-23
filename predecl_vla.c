
int vla1(int b, int a[*]);
int vla1(int b, int a[b]);

int vla1(int b, int a[b])
{
	return a[0] + a[b - 1];
}

int vla2(int a[*], int b);

int vla2(a, b)
	int b;
	int a[b];
{
	return a[0] + a[b - 1];
}
