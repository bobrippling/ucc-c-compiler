// RUN: %check -e %s

main()
{
	struct { int i : 3; } a;

	&a.i; // CHECK: /error: bitfield in address-of/
}
