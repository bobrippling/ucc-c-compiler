// RUN: %check %s -Wunused-function -Wunused-variable

int g();

static int f() // CHECK: warning: unused function 'f'
{
	g();
}

static int i; // CHECK: warning: unused variable 'i'
