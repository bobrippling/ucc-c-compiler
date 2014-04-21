// RUN: %ocheck 2 %s
// RUN: %check %s

f(hello); // CHECK: warning: old-style function declaration
// int hello

g(a) // CHECK: warning: old-style function declaration
{
	return a;
}

main()
{
	return f(5) + g(1); // 2
}

f(a) // CHECK: warning: old-style function declaration
	int a; // matches with int hello
{
	return a ? 1 : 0;
}
