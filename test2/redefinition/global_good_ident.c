// RUN: %check %s

extern int e; // CHECK: !/warn|error/
int e; // CHECK: !/warn|error/

static int f(void); // CHECK: !/warn|error/

int f() // CHECK: !/warn|error/
{ // CHECK: !/warn|error/
	return 3;
}
