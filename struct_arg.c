struct A
{
	int i, j;
};

f(struct A a)
{
	return a.i + a.j;
}

main()
{
	f((struct A){ 1, 2 });
}
