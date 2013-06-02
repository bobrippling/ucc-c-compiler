struct A
{
	int x : 3;
	int : 0; // forces y to align on a boundary
	int y : 4;
};
