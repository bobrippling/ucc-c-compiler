struct pair
{
	int m[2];
};

struct pair f(void);

main()
{
	return f().m[0]; // not an lvalue, can't `operator.'
}
