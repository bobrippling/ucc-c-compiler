// RUN: %ocheck 3 %s

f()
{
	return 3;
}

auto f_ret_f() -> int (*)()
{
	return f;
}

auto main() -> int
{
#include "../ocheck-init.c"
	return f_ret_f()();
}
