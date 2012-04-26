void inst(int i)
{
}

/*int *funopen(
		const void *,
		int    (*)(void *, char *, int),
		int    (*)(void *, const char *, int),
		fpos_t (*)(void *, fpos_t, int),
		int       (void *)
	);*/

main()
{
	void (*f)(int);

	f = inst;

	*f;
}
