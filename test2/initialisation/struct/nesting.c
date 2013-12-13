// RUN: %layout_check %s

struct
{
	struct
	{
		int i;
	} b;
	int j;
} a = { { 3 }, 2 },
  b = {   3  , 2 };
