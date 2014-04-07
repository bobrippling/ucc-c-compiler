int good(char ch[const *]); // CHECK: !/warn|error/
int bad(char ch[static *]); // CHECK: error: 'static' modifier

void bad2(char buf[*]) // CHECK: error: can't have star modifier
{
	(void)buf;
}

void good1(int *p, char buf[*p]); // CHECK: !/warn|error/

void good1(int *p, char buf[*p]) // CHECK: !/warn|error/
{
	(void)buf;
	(void)p;
}
