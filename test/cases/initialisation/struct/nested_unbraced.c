// RUN: %layout_check %s

struct
{
	struct
	{
		struct
		{
			struct
			{
				int i;
			} a;
		} b;
	} c;
} d = {
	1
};
