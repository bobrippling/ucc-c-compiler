// RUN: %check --only %s

int good(char ch[const *]);

void good1(int *p, char buf[*p]);

void good1(int *p, char buf[*p])
{
	(void)buf;
	(void)p;
}

void good2(int a[*]);

void good2(int a[])
{
	(void)a;
}

// [static i] is permitted here, but not for [static *]
void g(int i, double d[static i][i])
{
}
