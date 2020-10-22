// RUN: %check --only -e %s

int (*p)[] = &(int[]){1, 2, 3};

int (*q)[] = &(int[]){1, 2, 3};
int  *r[]  = &(int[]){1, 2, 3}; // CHECK: /error:/

int a[] = (int[]){1, 2, 3}; // CHECK: /error: .*must be initialised with an initialiser list or copy-assignment/

void f()
{
	static char  b[] = (int[]){1, 2, 3}; // CHECK: /error: .*must be initialised with an initialiser list or copy-assignment/
	// CHECK: ^error: "b" has incomplete type 'char[]'
	        char  *d = (int[]){1, 2, 3}; // CHECK: warning: mismatching types
	static char   *e = (int[]){1, 2, 3}; // CHECK: warning: mismatching types
}
