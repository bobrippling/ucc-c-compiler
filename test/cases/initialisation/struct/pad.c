// RUN: %layout_check %s
struct
{
	int *i; /* ######## */
	char c; /* #------- */
} a = { 0, 2 }, b = { 2, 3 };
