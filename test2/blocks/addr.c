// RUN: %check -e %s
main()
{
	&^{}; // CHECK: error: can't take the address of block
}
