// RUN: %ocheck 0 %s

struct store_out
{
	int *local;
	int *ret;
};

void store_out(const struct store_out *const so)
{
	*so->ret = *so->local;
}

void f(int *p)
{
	int i = *p;
	struct store_out so __attribute((cleanup(store_out))) = {
		.local = &i,
		.ret = p
	};

	i = 5;
}

main()
{
	int i = 3;

	f(&i);

	if(i != 5){
		void abort();
		abort();
	}

	return 0;
}
