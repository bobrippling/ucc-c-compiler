// RUN: %ucc -o %t %s && %t

#define false (_Bool)0

f(int a)
{
	if(a != 1)
		_Exit(1);

	return 3;
}

g(_Bool a){}

_Bool h()
{
	return 23;
}

int main()
{
	_Bool x = f((_Bool)5);

	g(false);

	if(h() != 1)
		return 1;

	return x == 1 ? 0 : 1;
}
