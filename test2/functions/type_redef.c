// RUN: %check -e %s

typedef int f; // CHECK: /error: mismatching definitions of "f"/

int f() // CHECK: /note: other definition/
{
	return 3;
}

int main()
{
	//f i;
	return f();
}
