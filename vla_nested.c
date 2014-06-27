#define L() printf("%s\n", __func__)

typedef void any(int);
typedef int ir(void);

any a, b, c;
ir x, y;

void a(int x){L();}
void b(int x){L();}
void c(int x){L();}

int x(){L();return 1;}
int y(){L();return 1;}

void f()
{
	int ar[x()][y()];

	a(sizeof ar);
	b(sizeof *ar);
	c(sizeof **ar);
}

main()
{
	f();
}
