// RUN: %check -e %s

typeof(__auto_type) b; // CHECK: error: __auto_type not wanted here

main()
{
	__auto_type brace = { 1 }; // CHECK: error: bad initialiser for __auto_type

	__auto_type array = { 1, 2 }; // CHECK: error: bad initialiser for __auto_type

	typedef __auto_type name; // CHECK: error: __auto_type without initialiser
	typedef __auto_type name2 = 3; // CHECK: error: initialised typedef

	int __auto_type z; // CHECK: error: can't combine __auto_type with previous type specifiers
	unsigned __auto_type q = 2u; // CHECK: error: __auto_type given with previous type specifiers
}
