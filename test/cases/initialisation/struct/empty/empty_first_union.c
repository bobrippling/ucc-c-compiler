// RUN: %layout_check %s
union A
{
	struct {};
	int i;
} a = { {}, 2 };
