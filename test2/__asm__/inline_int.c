// RUN: %archgen %s 'x86_64,x86:inline value $3'

__attribute((always_inline))
inline f(int i)
{
	__asm("inline value %0" : : "i"(i));
}

main()
{
	f(3);
}
