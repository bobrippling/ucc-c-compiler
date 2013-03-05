//typedef struct P9 {int p9; } P9;

struct A
{
	struct
	{
		int i, j;
	};
	//P9;
	int k;
};

struct A a = { .j = 1, 2, 3 };
//struct A a = { .i = 1, 2, 3 };
//struct A a = { .k = 1, 2, 3 };
//struct A a = { { 1, 2 }, 3 };
