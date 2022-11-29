// RUN: %ucc -fsyntax-only %s

__asm("hi");
main()
{
	return ({
		extern ev __asm("yo");
		ev;
	});
}

x __asm("yo");
__asm(".globl x\\nabc");
