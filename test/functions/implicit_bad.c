// RUN: %check -e %s

main()
{
	(f)(); // CHECK: /error: undeclared identifier "f"/
}
