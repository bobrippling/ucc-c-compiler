// RUN: %ucc -o %t %s && %t

main()
{
	switch(0)
	case 2:
	default:
		if(5)
			;
		;
		;
		;
		return 0;
}
