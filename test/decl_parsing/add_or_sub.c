// RUN: %ocheck 6 %s

call_for_me(int (*f)(int), int arg)
{
	return f(arg);
}

plus(int i)
{
	return i + 1;
}

sub(i)
{
	return i - 1;
}

int main()
{
	int (*f)(int);

	switch('+'){
		case '+': f = plus; break;
		case '-': f =  sub; break;
		default:  return 33;
	}

	return call_for_me(f, 5);
}
