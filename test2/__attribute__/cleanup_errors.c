int printf(char *, ...);
clean_cp(char **);
clean_ip(int **);

void clean(int *p __attribute((cleanup(clean_ip)))) // CHECK: warning: cleanup attribute only applies to local variables
{
	(void)p;
	printf("hi\n");
}

char *yo __attribute((cleanup(clean_cp))); // CHECK: warning: cleanup attribute only applies to local variables

void bad_cleaner1(void);
void bad_cleaner2(int, int);

static int g();
static void f() __attribute__((cleanup(g))); // CHECK: warning: cleanup attribute only applies to local variables

main()
{
	int func() __attribute__((cleanup(clean))); // CHECK: warning: cleanup attribute only applies to local variables
	static int y __attribute((cleanup(clean))); // CHECK: warning: cleanup attribute only applies to local variables

	int x[2] __attribute((cleanup(clean))); // CHECK: error: type 'int (*)[2]' passed - cleanup needs 'int *'

	int abc __attribute((cleanup(bad_cleaner1))); // CHECK: error: cleanup needs one argument (not 0)
	int xyz __attribute((cleanup(bad_cleaner2))); // CHECK: error: cleanup needs one argument (not 2)

	(void)y;
	(void)x;

	return 3;
}

self(int *p __attribute__((cleanup(self)))); // CHECK: error: function 'self' not found
