// RUN: %debug_check %s

f()
{
	int i = 2;
	for(int i = 90; i < 95; i++){
		int x = 10;
	}
	return i;
}

g(int x)
{
}

shadow()
{
	int local = 6;
	g(local);

	{
		g(local);
		int local = 7;
		g(local);
	}
}

main()
{
	int i = 1;
	int x = 5;

	while(i--){
		int hello;
		if(x == 5){
			int pad = 0;

			f();
		}
	}
}
