main()
{
	x(1 ? (struct A *)5 : 0); // should fold to 5
}
