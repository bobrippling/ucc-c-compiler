// RUN: %ucc %s
// RUN: %check %s

enum __attribute__((enum_bitmask)) e
{
	A, B, C
};

main()
{
	switch((enum e)0){ // CHECK: /warning: switch on enum with enum_bitmask attribute/
	}
}
