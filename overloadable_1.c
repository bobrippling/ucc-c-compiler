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

#ifdef IN_ONE
int f(int i, int j) __attribute__((overloadable))
{
	return i + j + 1;
}
#endif
