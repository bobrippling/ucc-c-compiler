enum
{
	A, B, C = B
};

main()
{
	switch((typeof(A))0){
		case 5:
		case B:
			;
	}
}
