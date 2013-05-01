// RUN: %ucc %s
// TODO: have %check ignore ifdef'd checks
// RUN: %ucc -DADDR   %s 2>&1 | %check %s
// RUN: %ucc -DSIZEOF %s 2>&1 | %check %s

main()
{
	struct { int i : 3; } a;

#ifdef ADDR
	&a.i; // CHECK: /error: taking the address of a bit-field/
#endif

	typeof(a.i) b; // CHECK: /warning: taking the address of a bit-field/

#ifdef SIZEOF
	return sizeof a.i; // CHECK: /error: sizeof applied to a bit-field/
#endif
}
