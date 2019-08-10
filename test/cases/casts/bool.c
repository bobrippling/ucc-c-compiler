// RUN: %ucc -o %t %s && %t
void _Exit(int) __attribute((noreturn));

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

int ar[(_Bool)5];

int main()
{
	_Bool x = f((_Bool)5);

	if(sizeof(ar) != 4)
		return 1;

	g(false);

	if(h() != 1)
		return 1;

	return x == 1 ? 0 : 1;
}
