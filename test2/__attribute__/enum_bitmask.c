// RUN: %ucc %s
// RUN: %check %s

enum e
{
	A, B, C
} __attribute__((enum_bitmask));

main()
{
	switch((enum e)0){ // CHECK: /warning: switch on enum with enum_bitmask attribute/
	}
}
