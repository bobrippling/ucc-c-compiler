typedef unsigned uint32_t;

uint32_t _rotr(uint32_t x, int n)
{
	// unsatisfiable:
	//__asm__("rorl %2, %0" : "=r"(x) : "0"(x), "Nc"(n));

	// bad codegen:
	__asm__("rorl %2, %0" : "=r"(x) : "r"(x), "Nc"(n));

	return x;
}
