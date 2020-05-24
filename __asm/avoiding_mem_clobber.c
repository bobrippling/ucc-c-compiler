#if 0
void gcc_recommended(char ptr[static 10])
{
	// https://gcc.gnu.org/pipermail/gcc/2014-September/215119.html
	// https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html

	asm(
			"// do something with 10 bytes of `ptr`"
			:
			: "m"(
				({
					struct {
						char x[10];
					} *p = (void *)ptr;
					*p;
				})
			)
	 );
}
#endif

#include <stdio.h>

int main(int argc, char* argv[])
{
	struct
	{
		int a;
		int b;
	} c;

	c.a = 1;
	c.b = 2;

	int Count = sizeof(c);
	void *Dest;

#if defined(EXAMPLE_1)
	__asm__ volatile (
			"rep; stosb"
			 : "=D" (Dest), "+c" (Count)
			 : "0" (&c), "a" (0)
#  ifdef FIXED
			 : "memory"
#  endif
	);

#elif defined(EXAMPLE_2)
  __asm__ __volatile__ (
			"rep; stosb"
       : "=D" (Dest)
			 , "+c" (Count)
			 , "+m" (*(struct foo { char x[8]; } *)&c) // no stmt-expr, direct lvalue
       : "0" (&c), "a" (0)
  );

#else
#  error need example

#endif

	printf("%u %u\n", c.a, c.b);
}
