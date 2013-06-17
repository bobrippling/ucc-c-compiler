struct A
{
	int n;
	int vals[];
} x[] = {
	1, { 1, 2, 3 } // array of struct-with-flexarray is disallowed
};
