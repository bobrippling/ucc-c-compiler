struct A
{
	int i : 2;
	int : 0;
	int j : 3;
	int : 4;
	int end : 7;
};

struct A x = {
	1, 2, 3; // should initialise i, j and k, skipping unnamed fields
};
