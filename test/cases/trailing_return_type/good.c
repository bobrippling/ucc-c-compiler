// RUN: %ucc -fsyntax-only %s

auto f() -> auto (*)() -> int;

void f2();

auto x -> long * = 3;

auto (*pf)() -> auto (*)(int) -> short = f2;

auto local(int nam) -> __typeof(x)
{
	return nam;
}

int main()
{
	((auto (*)() -> char)f)();

	pf()(3);
	f();

	f2();

	local(5);
}
