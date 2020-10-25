// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 1 ]

int f(int i){ return i + 1; }

main()
{
	__asm("movl $0, %edi\n"
			"call f\n"
			"ret\n");
	abort();
}
