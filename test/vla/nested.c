// RUN: %ocheck 0 %s
typedef void any(int);
typedef int ir(void);

any a, b, c;
ir x, y;

#define ONCE() static int c; if(c){abort();} c=1;

void a(int x){ONCE(); if(x != sizeof(int) * 2 * 3) abort(); }
void b(int x){ONCE(); if(x != sizeof(int) * 3) abort(); }
void c(int x){ONCE(); if(x != sizeof(int)) abort(); }

int x(){ONCE();return 2;}
int y(){ONCE();return 3;}

void test()
{
	int ar[x()][y()];

	a(sizeof ar);
	b(sizeof *ar);
	c(sizeof **ar);
}

main()
{
	test();

	return 0;
}
