struct A
{
	int x[5];
	short n;
} f()
{
	return (struct A){
		{ 1, 2, 3, 4, 5 }, 5
	};
}

main()
{
	int (*p)[5] = &f().x;
	f().x[1];
	*(f().x + 1);
}
