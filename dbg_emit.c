f(int a)
{
	void *x;

	x = g(a);

	if(a){
		return 0;
	}else{
		return -1;
	}

	/* no debug emitted here */
}
