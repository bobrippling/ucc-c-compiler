struct A { int init, i; };

//static struct A x;

q()
{
	static struct A a;

	if(!a.init){
		a.init = 1;
		a.i = 5;
	}else{
		++a.i;
	}

	return a.i;
}

main()
{
	for(int i = 0; i < 5; i++)
		printf("%d\n", q());
}
