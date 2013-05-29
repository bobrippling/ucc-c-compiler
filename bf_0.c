struct A
{
	int x : 3;
	int : 0; // forces y to align on a boundary
	int y : 4;
};

struct B
{
	int a : 6;
	int y : 0; // error
	int b : 2;
};
