// RUN: %check -e %s

main()
{
	struct { int i : 3; } a;

	return sizeof a.i; // CHECK: /error: sizeof applied to a bit-field/
}
