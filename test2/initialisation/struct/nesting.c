// RUN: %ucc -DO='{' -DC='}' -S -o- %s | %asmcheck %s
// RUN: %ucc -DO=    -DC=    -S -o- %s | %asmcheck %s

struct
{
	struct
	{
		int i;
	} b;
	int j;
} a = { O 3 C, 2 };
