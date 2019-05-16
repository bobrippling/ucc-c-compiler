// RUN: %ocheck 1 %s

int f(int i){ return i + 1; }

main()
{
	__asm("movl $0, %edi\n"
			"call f\n"
			"ret\n");
	abort();
}
