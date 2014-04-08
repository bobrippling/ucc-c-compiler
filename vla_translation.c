f(int n)
{
	short x[n];

	g(x, sizeof(x));

	return x[n - 1];
}

#if 0
// translate to:
f(int n)
{
	struct
	{
		short *p;
		size_t size;
	} x;
	// space for a short * and a size_t

	x->size = round_to_align(n * sizeof(x[0]));
	$sp -= alloca(x->size);
	x->p = $sp;

	g(x->p, x->size);

	$ret = x->p[n - 1];
	$sp += x->size;
	return $ret;
}
#endif
