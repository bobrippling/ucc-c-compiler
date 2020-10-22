// RUN: %ucc %s -o %t
// RUN: %t; [ $? -eq 3 ]

main()
{
	__asm("movl $3, %eax");
}
