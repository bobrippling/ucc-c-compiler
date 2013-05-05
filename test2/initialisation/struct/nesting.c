// RUN: %ucc -DO='{' -DC='}' -S -o- %s | %asmcheck %s
// RUN: %ucc -DO=    -DC=    -S -o- %s | %asmcheck %s

#ifndef O
#  define O {
#endif
#ifndef C
#  define C }
#endif

struct
{
	struct
	{
		int i;
	} b;
	int j;
} a = { O 3 C, 2 };
