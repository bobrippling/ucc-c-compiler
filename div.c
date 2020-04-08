/* Commuting inputs */
unsigned func12(unsigned p1, unsigned p2)
{
	unsigned hi, lo;

	asm ("mul %3"
		: "=a" (lo), "=d" (hi)
		: "%0" (p1), "r" (p2) : "cc"); /* %0 - '%' means it commutes with the next operand */

	return hi + lo;
}
