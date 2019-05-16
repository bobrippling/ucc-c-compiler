// RUN: ! %ucc %s -fno-builtin
// RUN: %ucc %s

main()
{
	_Static_assert(__builtin_constant_p(strlen("hi")), "no builtin");
}
