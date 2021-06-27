// RUN: %layout_check %s

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
