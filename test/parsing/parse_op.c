// RUN: %check %s

a(), b(), c();

f()
{
	return a() & (b() == c()); // CHECK: !/warning/
}

main()
{
	return a() & b() == c(); // CHECK: warning: == has higher precedence than &
}
