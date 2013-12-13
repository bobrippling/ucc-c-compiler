// RUN: %layout_check %s

struct A
{
	int i, j;
} f[] = {
	{ 1, 2 },
	{ .j = 4, .i = 5 }
};

int x[4] = { 1, 2, 3 };

int y[] = { 1, 2, 3 };

struct
{
	int x[4];
}
a = {
	.x = {
		[1] = 3
	}
},
b = {
	.x[1] = 3
},
c = {
	.x = 3
},
d = {
	.x[1] = 3
},
e = {
	.x[1] = { 3 }
};
