// RUN: %check %s

int good(char ch[const *]); // CHECK: !/warn|error/

void good1(int *p, char buf[*p]); // CHECK: !/warn|error/

void good1(int *p, char buf[*p]) // CHECK: !/warn|error/
{
	(void)buf;
	(void)p;
}

void good2(int a[*]);

void good2(int a[]) // CHECK: !/warn|error/
{
	(void)a;
}
