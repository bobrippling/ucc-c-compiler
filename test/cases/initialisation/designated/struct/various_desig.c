// RUN: %layout_check %s

struct
{
	int i, j;
}

a = { 1, 2 },

b = { .i = 3, 4 },

c = { .j = 5, 32 },

d = { .j = 7, .i = 6, },

e = { .i = 5, .i = 6, };
