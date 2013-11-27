// RUN: %check -e %s

int (*p)[] = &(int[]){1, 2, 3}; // CHECK: !/error:/

int (*q)[] = &(int[]){1, 2, 3}; // CHECK: !/error:/
int  *r[]  = &(int[]){1, 2, 3}; // CHECK: /error:/

f()
{
	static char  b[] = (int[]){1, 2, 3}; // CHECK: /error:/
	        char  *d = (int[]){1, 2, 3}; // CHECK: !/error:/
	static char   *e = (int[]){1, 2, 3}; // CHECK: !/error:/
}

int a[] = (int[]){1, 2, 3}; // CHECK: /error:/
