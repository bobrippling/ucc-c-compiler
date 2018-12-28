// RUN: ! %ucc %s

struct
{
	int x[4];
	int y[4];
} a = {
	.x[1] = 1,
	2,
	3,
	[2] = 5 // error
};

