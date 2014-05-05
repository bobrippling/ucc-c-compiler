int main()
{
	auto f() -> int (*)();

	auto (*pf)() -> auto (*)(int) -> int = 0;
	return pf()(2);
}
