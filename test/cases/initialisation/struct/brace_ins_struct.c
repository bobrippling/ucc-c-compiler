// RUN: %layout_check %s
struct A
{
	struct B
	{
		struct C
		{
			int i;
		} c;
	} b;
} ent1 = {
	// insert
	// insert
	// insert
	1
	// finish
	// finish
	// finish
};
