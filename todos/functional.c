static list map(void (^func)(), list in)
{
	for(val x in in)
		func(x);
	return in;
}

static list filter(int (^func)(), list in)
{
	list ret = list_new();

	for(val x in in)
		if(func(x))
			ret.add(x);

	return ret;
}

static val fold(val begin, val (^func)(), list in)
{
	for(val x in in)
		begin = func(begin, x);
	return begin;
}

void run()
{
	map(   ^(entry a){ printf("%d\n", a); }, vals);
	filter(^(entry a){ return a != 0;     }, vals);

	val sum = fold(1,
			^(val current, val n){
				return current + n;
			},
			list_new(1, 2, 3));

	printf("sum = %d\n", sum);
}
