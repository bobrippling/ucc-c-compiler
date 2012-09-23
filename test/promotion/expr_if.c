main()
{
	struct A { int j, i; };// *p;
	//void *p = (__typeof(5)*)5;

	/*
	printf("%d %d\n",
			((struct { int i; } *)p)->i,
			(p ? NULL : (struct{int i;}*)3)->i);
	*/

	return
		(1 ? (struct A *)0 :     (void *)0)->i
	+ (1 ? (void *)0     : (struct A *)0)->j;
}
