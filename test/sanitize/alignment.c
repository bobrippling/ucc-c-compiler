// RUN: %ocheck 5 %s -fsanitize=alignment -fsanitize-error=call=san_err

void san_err(void)
{
	__attribute((noreturn))
	void exit(int);
	exit(5);
}

int f(int *p)
{
	return *p;
}

int main()
{
	int a = 3;
	f((char *)&a + 2);
};
