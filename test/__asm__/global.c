// RUN: %ucc -S -o- %s

__asm("hi");
main()
{
	return ({extern timmay __asm("yo"); timmay;});
}

x __asm("yo");
__asm(".globl x\\nhai");
