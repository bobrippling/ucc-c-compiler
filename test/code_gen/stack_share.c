// RUN: %ocheck 0 %s
// RUN: %ocheck 0 %s -fstack-protector-all

void abort();

check(int x)
{
	if(!x)
		abort();
}

g(int *p)
{
	static int i;
	switch(i++){
		case 0:
			check(p[0] == 3);
			check(p[1] == 0);
			check(p[2] == 1);
			break;
		case 1:
			check(p[0] == 2);
			check(p[1] == 5);
			check(p[2] == 9);
			break;
		default:
			abort();
	}
}

h(char *p)
{
	static int called;

	if(called)
		abort();

	if(strcmp(p, "hi"))
		abort();

	called = 1;
}

f(int c, int d)
{
	if(c){
		int x[] = { 3, 0, 1 };
		g(x);
	}else{
		int y[] = { 2, 5, 9 };
		g(y);
	}

	if(d){
		char s[] = "hi";
		h(s);
	}
}

shared()
{
	int *p, *q;

	// UB - but we define this exact case to share 'abc' space
	{
		int abc;
		p = &abc;
	}
	{
		int abc;
		q = &abc;
	}

	if(p != q)
		abort();
}

main()
{
	f(1, 1);
	shared();
	f(0, 0);
}
