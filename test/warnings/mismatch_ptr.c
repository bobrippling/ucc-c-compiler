typedef void func(void);

int main()
{
	func *fn = 0;
	void *v = 0;

	fn = v;
	v = fn;
}
