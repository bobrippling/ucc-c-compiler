// RUN: echo TODO %s
// RUN: false

int *p[] = &(int[]){1, 2, 3}; // yes

int   a[]  =  (int[]){1, 2, 3}; // yes
int (*p)[] = &(int[]){1, 2, 3}; // yes
int  *p[]  = &(int[]){1, 2, 3}; // no

f()
{
	static int   a[] = (int[]){1, 2, 3}; // yes
	static char  b[] = (int[]){1, 2, 3}; // no
	       char  c[] = (int[]){1, 2, 3}; // no
	        char  *d = (int[]){1, 2, 3}; // yes
	static char   *e = (int[]){1, 2, 3}; // no
}
