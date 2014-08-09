__attribute((always_inline, noinline))
int f()
{
	return 3;
}

int main()
{
	return f();
}
