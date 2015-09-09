void g(char *, int *);

void f()
{
	char buf[512];
	int i = 2;

	g(buf, &i);
}
