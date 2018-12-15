// RUN: %check -e %s

int f(int a,); // CHECK: error: parameter expected
int f(int,, int); // CHECK: error: parameter expected

main()
{
	f(1, 3);
}
