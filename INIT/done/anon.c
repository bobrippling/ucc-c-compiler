typedef struct P9 {int p9; } P9;

struct A
{
	struct
	{
		int i, j;
	};
	P9;
	int k;
};

struct A a = { .j = 1, .k = 2, .i = 0 }; // 0, 1, 0, 2
struct A b = { .p9 = 2, .k = 3 }; // 0, 0, 2, 3
struct A c = { .i = 1, 2, 3 }; // 1, 2, 3, 0
struct A d = { .k = 1, 2, 3 }; // 0, 0, 0, 1
struct A e = { { 1, 2 }, 3 }; // 1, 2, 3, 0
