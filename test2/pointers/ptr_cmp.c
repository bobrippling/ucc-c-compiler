// RUN: %ucc %s
// RUN: %ucc -c %s 2>&1 | %check %s

main()
{
	return (char *)0 == (int *)5; // CHECK: /warning: comparison of distinct pointer types lacks a cast/
}
