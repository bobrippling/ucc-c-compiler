typedef struct
{
	int a, b, c;
} P9;

struct A
{
	int out;
	P9;
} x = {
	1,2,3,4,
	.P9.b = 5
};
