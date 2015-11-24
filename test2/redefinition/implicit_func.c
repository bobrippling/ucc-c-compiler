// RUN: %check -e %s

int main()
{
	{
		f(); // CHECK: warning: implicit declaration of function "f"
	// CHECK: ^ note: previous definition
	}
	{
		f("hi");
	}
}

char f() // CHECK: error: mismatching definitions of "f"
{
	return 3;
}
