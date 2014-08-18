typedef short unsigned uint16_t;
typedef long  unsigned uint64_t;

main()
{
	struct __attribute__((packed))
	{
		uint16_t limit;
		uint64_t addr;
		char buf[256];
	} idt;

	asm ("sidt %0" : "=r" (idt));
}
