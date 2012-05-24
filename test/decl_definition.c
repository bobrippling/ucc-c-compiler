extern int i = 5; // extern removed

static int j = 2;
int j; // error

main()
{
	int f(), g();
	//int h();

	f(g(h()));
}

int k = 5;
static int k = 2; // error


extern int m;
int m; // m is implicit def

extern int n; // ignored
int n = 5;
int l;
extern int l; // ignored

int p = 2;
int p = 2; // error
