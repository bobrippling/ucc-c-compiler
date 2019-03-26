// RUN: %check %s

enum __attribute__((enum_bitmask)) e
{
	A, B, C
};

enum __attribute__((flag_enum)) e2
{
	X,
};


main()
{
	switch((enum e)0){ // CHECK: warning: switch on enum with enum_bitmask attribute
	}
	switch((enum e2)0){ // CHECK: warning: switch on enum with enum_bitmask attribute
	}
}
