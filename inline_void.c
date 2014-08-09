void f(int const *p)
{
	*(int *)p = 3;
}

int main()
{
	int i;
	f(&i);
	return i;
}
