// RUN: %check %s

main()
{
	struct { int m : 5; } a;
	a.m = 99999; // CHECK: /warning: truncation in store to bitfield alters value: 99999 -> 31/
}
