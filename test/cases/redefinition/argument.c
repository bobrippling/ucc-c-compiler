// RUN: %check -e %s -Wshadow

void f(int n) // CHECK: !/error/
{
	{
		int n = 2; // CHECK: !/error/
	}
}

void g(int n) // CHECK: error: duplicate definitions of "n"
{
	int n = 1; // CHECK: note: previous definition
	{
		int n = 2; // CHECK: !/error/
	}
}
