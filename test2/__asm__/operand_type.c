// RUN: %archgen %s 'x86,x86_64:reg=%%rax' -Dty=long
// RUN: %archgen %s 'x86,x86_64:reg=%%eax' -Dty=int
// RUN: %archgen %s 'x86,x86_64:reg=%%ax' -Dty=short
// RUN: %archgen %s 'x86,x86_64:reg=%%al' -Dty=char

main()
{
	ty v;

	__asm("reg=%0" : "=r"(v));
}
