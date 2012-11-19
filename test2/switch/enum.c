enum A
{
	A,
	B,
	C = A
};

main()
{
	switch((enum A)0){
		case A:
		case B:
		case C:
			;
	}
}
