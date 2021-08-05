#ifndef IMPL
int f1(int a){return a;}

int f2(int a, int b){return a;}
int f3(int a, int b){return b;}

int f4(int a, int b, int c){return a;}
int f5(int a, int b, int c){return b;}
int f6(int a, int b, int c){return c;}

int f7(int a, int b, int c, int d){return a;}
int f8(int a, int b, int c, int d){return b;}
int f9(int a, int b, int c, int d){return c;}
int f10(int a, int b, int c, int d){return d;}
#else
int f1(int a);

int f2(int a, int b);
int f3(int a, int b);

int f4(int a, int b, int c);
int f5(int a, int b, int c);
int f6(int a, int b, int c);

int f7(int a, int b, int c, int d);
int f8(int a, int b, int c, int d);
int f9(int a, int b, int c, int d);
int f10(int a, int b, int c, int d);
#endif

#ifdef IMPL
void assert(_Bool x, int lno)
{
	int printf(const char *, ...);

	printf(__FILE__ ":%d: %s\n", lno, x ? "pass" : "\x1b[31mfail\x1b[0m");
}
#define assert(x) assert(x, __LINE__)

int main()
{
	assert(f1(3) == 3);

	assert(f2(5, 9) == 5);
	assert(f3(5, 9) == 9);

	assert(f4(5, 9, 14) == 5);
	assert(f5(5, 9, 14) == 9);
	assert(f6(5, 9, 14) == 14);

	assert(f7(5, 9, 14, 26) == 5);
	assert(f8(5, 9, 14, 26) == 9);
	assert(f9(5, 9, 14, 26) == 14);
	assert(f10(5, 9, 14, 26) == 26);
}
#endif
