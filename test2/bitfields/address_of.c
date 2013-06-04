// RUN: %check -e %s

main()
{
	struct { int i : 3; } a;

	&a.i; // CHECK: /error: taking the address of a bit-field/
}
