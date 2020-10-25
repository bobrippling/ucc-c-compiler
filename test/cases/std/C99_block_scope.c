// RUN: %ucc -std=c99 %s -o %t
// RUN: %t; [ $? -eq 4 ]

// RUN: %ucc -std=c90 %s -o %t
// RUN: %t; [ $? -eq 8 ]

// RUN: %ucc -std=c89 %s -o %t
// RUN: %t; [ $? -eq 8 ]

struct A { int i; };

main()
{
	if((struct A { int i, j; } *)0)
		;

	return sizeof(struct A);
}
