// RUN: %check -e %s

auto f() -> auto (*)() -> int;

auto (*pf)() -> auto (*)(int) -> short = f;

int main()
{
	pf()()(); // CHECK: error: too few arguments to function (got 0, need 1)
	f()()(); // CHECK: error: funcall-expression (type 'int') not callable
}
