// RUN: %check -e %s

f()
{
}

int main()
{
	{
		__label__ abc;
		goto abc;

	abc:
		f();
	}

	goto abc; // CHECK: error: label 'abc' undefined

	{
		// declared local but not defined
		__label__ x; // CHECK: error: label 'x' undefined

q: // CHECK: warning: unused label 'q'
		f();
	}

	{
		__label__ x; // CHECK: error: label 'x' undefined
		// declared local but not defined

		goto x; // not defined
	}
}
