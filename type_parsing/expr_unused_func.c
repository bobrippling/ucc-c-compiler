void *x(int i){} // the pointer breaks this

main()
{
	x; // should load the address of x
	&x;
}
