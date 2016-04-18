// RUN: %ucc '-Dcheck(a,b)=_Static_assert((a) == (b), "")' -fsyntax-only %s
// RUN: %ucc '-Dcheck=assert_eq' %s -o %t
// RUN: %t

void abort(void);

void assert_eq(int a, int b)
{
	if(a != b)
		abort();
}

int main()
{
	enum e { c = 1u };

	check(0, c < -1);
	check(1, (enum e)c < -1);

	return 0;
}
