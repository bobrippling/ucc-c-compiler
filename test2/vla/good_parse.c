// RUN: %check %s

int good(char ch[const *]); // CHECK: !/warn|error/

void good1(int *p, char buf[*p]); // CHECK: !/warn|error/

void good1(int *p, char buf[*p]) // CHECK: !/warn|error/
{
	(void)buf;
	(void)p;
}
