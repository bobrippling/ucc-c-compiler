// RUN: %check -e %s

main()
{
	struct { int i : 3; } a;

	typeof(a.i) b; // CHECK: /warning: typeof applied to a bit-field/

	return sizeof a.i; // CHECK: /error: sizeof applied to a bit-field/
}
