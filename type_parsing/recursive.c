int x(int (*self)())
{
	printf("hi\n");
	self(self);
}

main()
{
	x(x);
}
