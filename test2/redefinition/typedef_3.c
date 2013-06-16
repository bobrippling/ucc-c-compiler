// RUN: %ucc -c %s
h()
{
	typedef int t;
	{
		int t;
		t = 2;
		return t;
	}
}
