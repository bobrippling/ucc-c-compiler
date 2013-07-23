g(double d)
{
	void f(float);

	f(d); // trying to trigger assert in x86_xchg_fi
}
