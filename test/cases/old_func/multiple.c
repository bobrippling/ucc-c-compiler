// RUN: %check -e %s

f(hello); // CHECK: warning: old-style function declaration
// CHECK: ^note: previous definition

g(a) // CHECK: warning: old-style function declaration
{
	return a;
}

main()
{
	return f(5) + g(1); // 2
}

f(a) // CHECK: warning: old-style function declaration
	// CHECK: ^error: mismatching definitions of "f"
	char *a;
{
	return a ? 1 : 0;
}
