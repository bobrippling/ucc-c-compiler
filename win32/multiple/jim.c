void f(int);

int main()
{
	void (*p)(int) = f;

	printf("hai there");
	for(int i = 0; i < 3; i++)
		p(i);
	f(0);

	return 42;
}
