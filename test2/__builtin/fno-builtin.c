// RUN: %ucc -fno-builtin %s -o %t; %t; [ $? -ne 0 ]
// RUN: %ucc              %s -o %t; %t; [ $? -eq 0 ]

main()
{
	_Static_assert(__builtin_constant_p(strlen("hi")), "no builtin");
}
