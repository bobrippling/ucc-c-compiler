// RUN: %ucc -fno-builtin %s -o %t; [ $? -ne 0 ]
// RUN: %ucc              %s -o %t

main()
{
	_Static_assert(__builtin_constant_p(strlen("hi")), "no builtin");
}
