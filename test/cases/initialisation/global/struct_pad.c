// RUN: %layout_check %s
struct A
{
	char a; /* b c d */
	long l;
	int i;
} a = { 1, 2 };

/*
.byte   1, .space 7
.quad   2
.space  4, .space 4
*/
