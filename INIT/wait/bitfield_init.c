struct
{
	int x;
	int y : 5;
	unsigned b : 1;
	int : 20;
	int end;
} a = {
	1,2,0,4
};
