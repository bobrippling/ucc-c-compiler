// RUN: %check %s

main()
{
	char x[200];
	return sizeof(0, x); // CHECK: /warning: array parameter size is /
}
