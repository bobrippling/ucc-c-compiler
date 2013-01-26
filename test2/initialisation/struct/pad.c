// RUN: %ucc -S -o- %s | %asmcheck %s
struct
{
	int *i; /* ######## */
	char c; /* #------- */
} a = { 0, 2 }, b = { 2, 3 };
