// RUN: %layout_check %s
union A
{
	int i;
	struct {};
} a = { 1 };
