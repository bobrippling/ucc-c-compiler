// RUN: %check %s

main()
{
	int ar[] = { [0 ... 9] = 3 }; // CHECK: !/warn.*comma/
}
