main()
{
	switch(2){
		case 2:
			switch(5 - 2){
				case 3:
					break;
				case 10:
					abort();
			}
			break;

		case 1:
		default:
			abort();
	}
	return 0;
}
