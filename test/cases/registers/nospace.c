// RUN: %ucc %s
// we don't do register assignment atm
main()
{
	register int i __asm("edi");

	return i;
}
