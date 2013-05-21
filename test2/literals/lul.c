// RUN: %check -e %s
main()
{
	return 1LUL; // CHECK: /error: bad suffix "LUL"/
}
