// RUN: %check -e %s

main()
{
	int i = j, j = 0; // CHECK: error: undeclared identifier "j"
}
