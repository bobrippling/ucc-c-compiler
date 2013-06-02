// RUN: %ucc %s
// TODO: have %check ignore ifdef'd checks
// RUN: %check %s -DADDR
// RUN: %check %s -DSIZEOF

main()
{
	struct { int i : 3; } a;

#ifdef ADDR
	&a.i; // CHECK: /error: taking the address of a bit-field/
#endif

	typeof(a.i) b; // CHECK: /warning: typeof a bitfield/

#ifdef SIZEOF
	return sizeof a.i; // CHECK: /error: sizeof applied to a bit-field/
#endif
}
