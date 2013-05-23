// RUN: %check %s

#define TY_ASSERT(n, ty)            \
	_Static_assert(                   \
			__builtin_types_compatible_p( \
				typeof(n), ty),             \
			#n "'s type isn't " #ty)

#define TY_ASSERT2(n, ty_a, ty_b)                       \
	_Static_assert(                                       \
			   __builtin_types_compatible_p(typeof(n), ty_a)  \
			|| __builtin_types_compatible_p(typeof(n), ty_b), \
			#n "'s type isn't " #ty_a " or " #ty_b)

// automatic promotions
TY_ASSERT(-1, signed);

TY_ASSERT(0x7fffffff,   signed);
TY_ASSERT(0xffffffff, unsigned);

TY_ASSERT(0x100000000, long); // low limit
TY_ASSERT(0x7fffffffffffffff, long); // high limit
TY_ASSERT(0xffffffffffffffff, unsigned long);

TY_ASSERT(0x10000000000000000, unsigned long long); // CHECK: /overflow parsing integer/

// explicit suffixes
TY_ASSERT(1L, long);
TY_ASSERT(1U, unsigned);
TY_ASSERT(1UL, unsigned long);
TY_ASSERT(0xffffffffL, long);
TY_ASSERT(0xffffffffLU, unsigned long);
TY_ASSERT(0xffffffffffUL, unsigned long);


TY_ASSERT(0x7fffffffffffffffU, unsigned long); // no L-suffix, but promoted
TY_ASSERT(0x7fffffffffffffffL,          long); // no U-suffix, signed
TY_ASSERT(0xffffffffffffffffL, unsigned long); // no U-suffix, but promoted

TY_ASSERT2(0x10000000000000000L, unsigned long, unsigned long long); // L-suffix, but promoted // CHECK: /warning: overflow parsing integer/
TY_ASSERT2(0xfffffffffffffffffffffffffffL, unsigned long, unsigned long long); // L-suffix, but promoted since it's too small // CHECK: /warning: overflow parsing integer/

TY_ASSERT(2147483647,   signed);
TY_ASSERT(4294967295,   long);
TY_ASSERT(4294967295L,  long);
TY_ASSERT(4294967295LU, unsigned long);
TY_ASSERT(4294967296,   long);
TY_ASSERT(1099511627775UL,     unsigned long);
TY_ASSERT(9223372036854775807, long);
TY_ASSERT(9223372036854775807U, unsigned long);
TY_ASSERT(9223372036854775807L,          long);
TY_ASSERT2(18446744073709551615, unsigned long, unsigned long long); // CHECK: /warning: integer constant is so large it is unsigned/
TY_ASSERT(18446744073709551615L, unsigned long long); // CHECK: /warning: integer constant is so large it is unsigned/

TY_ASSERT(1LL, long long);
TY_ASSERT(01LL, long long);
TY_ASSERT(1LLU, unsigned long long);
TY_ASSERT(1ULL, unsigned long long);
