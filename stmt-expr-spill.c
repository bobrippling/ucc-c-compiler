void g(int *);

int f()
{
	return ({
		int x __attribute((cleanup(g)));
		q(&x);
		x;
	}) + 3;
}
