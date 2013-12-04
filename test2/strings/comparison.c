// RUN: %check %s

f(char *b)
{
	return "y" > b; // CHECK: warning: comparison with string literal is undefined
}

main()
{
	return "a" == "a"; // CHECK: warning: comparison with string literal is undefined
}
