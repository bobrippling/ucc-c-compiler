// RUN: %ocheck 4 %s -fsanitize=bool -fsanitize-error=call=san_err

enum { false, true };

int ec = 0;

void san_err(void)
{
	void exit(int) __attribute((noreturn));
	exit(ec);
}

int load_bool(_Bool *p)
{
	return *p;
}

int main()
{
	load_bool(&(_Bool){ false });
	load_bool(&(_Bool){ true });

	ec = 4;

	typedef char anybool[sizeof(_Bool)] __attribute((aligned(_Alignof(_Bool))));
	anybool b = { 3 };
	load_bool(&b);

	ec = 0;
	return ec;
}
