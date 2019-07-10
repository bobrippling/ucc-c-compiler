// RUN: %ocheck 5 %s

f(void (^*blk)(void))
{
	(*blk)();
}

void exit(int);

int main()
{
	__auto_type b = ^{ exit(5); };
	f(&b);

	return 0;
}
