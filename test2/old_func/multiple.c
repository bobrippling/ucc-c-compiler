// RUN: %check %s
// RUN: %ocheck 2 %s

f(hello); // CHECK: /warning: old-style function declaration/

g(a)
{
	return a;
}

main()
{
	return f(5) + g(1); // 2
}

f(a)
	char *a;
{
	return a ? 1 : 0;
}
