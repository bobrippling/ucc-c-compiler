// RUN: %ucc -DCHK='A == 2' -std=c89 -o %t %s && %t
// RUN: %ucc -DCHK='__builtin_types_compatible_p(typeof(exceed), long)' -std=c99 -o %t %s && %t

// RUN: %ucc -DCHK='__builtin_is_signed(enum neg)' -o %t %s && %t
// RUN: %ucc -DCHK='!__builtin_is_signed(enum usigned)' -o %t %s && %t

enum exceed
{
	A = 2147483647 + 3 // C89 - warning, value is `2', aka that % INT_MAX
		                 // C99 - type becomes `long'
};

enum neg
{
	X = 0,
	Y = -1
}; // C99 - neg is signed. e.g. signed char, but X and Y are still `int'

enum usigned
{
	H, J, K
};

main()
{
	return (CHK) ? 0 : 32;
}
