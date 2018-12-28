// RUN: %layout_check %s -fplan9-extensions

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

struct A ent1 = { .j = 1, .k = 2, .i = 0 }; // 0, 1, 0, 2

struct A ent2 = { .p9 = 2, .k = 3 }; // 0, 0, 2, 3
struct A ent3 = { .p9 = 2, 3 }; // 0, 0, 2, 3

struct A ent4 = { .i = 1, 2, 3 }; // 1, 2, 3, 0
struct A ent5 = { .k = 1, 2, 3 }; // 0, 0, 0, 1
struct A ent6 = { { 1, 2 }, 3 }; // 1, 2, 3, 0
