// RUN: %ucc -S -o- %s

asm("hi");
main()
{
	return ({extern timmay asm("yo"); timmay;});
}

x asm("yo");
asm(".globl x\\nhai");
