// RUN: %check %s

f(float);

main()
{
	f(1); // CHECK: !/warn/
	return 0;
}
