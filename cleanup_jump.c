int *pr;
clean(int *p)
{
	*pr = *p;
}

setup(int *r)
{
	pr = r;
}

f(int a)
{
	int r;

	setup(&r);

	{
		int __attribute((cleanup(clean))) i;
		switch(a){
			case 0:
				i = g();
				goto end;
			case 1:
				i = g() + g();
				goto end;
			case 2:
				i = 0;
				goto end;
			case 3:
				i = h();
				goto end;
			default:
				i = 6;
				break;
		}
	}

	skipped();

end:
	return r;
}
