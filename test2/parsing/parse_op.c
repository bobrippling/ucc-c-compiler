// RUN: %ucc -c %s
// RUN: %ucc -c %s 2>&1 | %check %s

f()
{
	return a() & (b() == c()); // CHECK: !/warning/
}

main()
{
	return a() & b() == c(); // CHECK: warning: == has higher precedence than &
}
