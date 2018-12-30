int x(int (*self)())
{
	static int x;
	if(++x > 10){
		printf("limit\n");
		exit(0);
	}
	printf("hi\n");
	self(self);
}

main()
{
	x(x);
	return 0;
}
