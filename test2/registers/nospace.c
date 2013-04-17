// RUN: %ucc %s
// we don't do register assignment atm
main()
{
	register int i asm("edi");

	return i;
}
