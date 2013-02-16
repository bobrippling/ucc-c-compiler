// RUN: %ucc -fno-builtin %s; [ $? -ne 0 ]
// RUN: %ucc              %s; [ $? -eq 0 ]

main()
{
	_Static_assert(__builtin_constant_p(strlen("hi")), "no builtin");
}
