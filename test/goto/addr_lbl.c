// RUN: %ocheck 3 %s

main()
{
	void *p = &&x;

rejmp:
	goto *p;
x:
	p = &&y;
	goto rejmp;
y:
	return 3;
}

f()
{
y:;
}
