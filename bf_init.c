struct Basic
{
	int x : 4, y : 4;
} bas = { 1, 2 };

struct Desig
{
	int x : 16, y : 2;
} des = {
	.y = 1
};

struct Padded
{
	int i : 2;
	int : 0;
	int j : 3;
	int : 4;
	int end : 13;
};

struct Padded pad = {
	1, 2, 3 // should initialise i, j and k, skipping unnamed fields
};
