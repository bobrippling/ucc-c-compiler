// RUN: %jmpcheck %s

f(int a)
{
	if(0){
		int x;
		g();
l:
		x = 3;
		return x;
	}

	if(a)
		goto l;
	return 5;
}
