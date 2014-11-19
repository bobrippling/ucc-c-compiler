// RUN: %ucc -g %s -o %t

f(int a)
{
	void *x = 0;
	(void)x;

	if(a){
		return 0;
	}else{
		return -1;
	}

	/* no debug emitted here */
}

main()
{
	f(0);
}
