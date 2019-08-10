// RUN: %ucc %s -c

int *f(int *p)
{
	char c = 0;
	//short s = 0;

	return p + c;
}

unsigned *g(unsigned *p)
{
	unsigned char c = 0;
	//short s = 0;

	return p + c;
}

unsigned long h(unsigned char *p)
{
	return *p;
}

long f2(int *p)
{
	return *p;
}
