typedef int i;

void foo()
{
	struct i {i i;} i;
i: i.i = 3;
	 printf( "%i\n", i.i);
}
