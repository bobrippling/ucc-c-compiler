(*f())()
{
	extern f();
	return f;
}
