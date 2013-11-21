// RUN: %check -e %s

int (*p)[] = &(int[]){1, 2, 3}; // CHECK: !/error:/

int   a[]  =  (int[]){1, 2, 3}; // CHECK: /error:/
int (*p)[] = &(int[]){1, 2, 3}; // CHECK: !/error:/
int  *p[]  = &(int[]){1, 2, 3}; // CHECK: /error:/

f()
{
	static int   a[] = (int[]){1, 2, 3}; // CHECK: /error:/
	static char  b[] = (int[]){1, 2, 3}; // CHECK: /error:/
	       char  c[] = (int[]){1, 2, 3}; // CHECK: /error:/
	        char  *d = (int[]){1, 2, 3}; // CHECK: !/error:/
	static char   *e = (int[]){1, 2, 3}; // CHECK: !/error:/
}
