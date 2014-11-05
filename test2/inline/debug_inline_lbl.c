// RUN: %ucc -g %s

__attribute((always_inline))
_Noreturn inline void abort()
{
	int i;
	{
		int j;
		__builtin_unreachable();
	}
}

__attribute((always_inline))
inline void realloc()
{
	int local = 5;
	{
		int local2 = 3;
		abort();
		/* inlined unreachable code, inline-end-label (for debug info)
		 * still needs to be emitted. this tests it is, or at least
		 * that there aren't linker errors afterwards
		 */
	}
}

main()
{
	realloc();
}
