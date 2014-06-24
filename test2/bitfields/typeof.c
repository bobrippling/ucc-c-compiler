// RUN: %check -e %s

main()
{
	struct { int i : 3; } a;

	__typeof(a.i) b; // CHECK: /error: bitfield in typeof/
}
