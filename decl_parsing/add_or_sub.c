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

int main(int argc, char **argv)
{
	int (*f)(int);

	if(argc != 2)
		return 53;

	if(argv[1][1] != '\0')
		return 43;

	switch(argv[1][0]){
		case '+': f = plus; break;
		case '-': f =  sub; break;
		default:  return 33;
	}

	return call_for_me(f, 5);
}
