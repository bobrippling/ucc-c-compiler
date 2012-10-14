void f();

int main()
{
	void (*p)() = f;

	printf("hai there");
	for(int i = 0; i < 3; i++)
		f();
	p();
}
