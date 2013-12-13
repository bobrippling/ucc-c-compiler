// RUN: %check -e %s

typedef int f; // CHECK: /note: previous definition/

int f() // CHECK: /error: mismatching definitions of "f"/
{
	return 3;
}

int main()
{
	//f i;
	return f();
}
