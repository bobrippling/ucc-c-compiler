main()
{
	void *p = &&a;
	void **pp = &p;

	goto **pp;
a:
	;
}
