int f(int i) __attribute__((overloadable))
{
	//return i + 1;
}

int f(int i, char (*af)()) __attribute__((overloadable))
{
	//return i + af();
}

int main()
{
	f(2, 4);
}
