char **filter(char **n, int (^f)(char *))
{
	int len, sz, i, j;
	char **r;

	len = 0;
	for(i = 0; n[i]; i++, len++);

	sz = (len + 1) * sizeof *r;
	r = malloc(sz);
	memset(r, 0, sz);

	for(j = i = 0; n[i]; i++)
		if(f(n[i]))
			r[j++] = n[i];

	r[j] = 0;

	return r;
}

main()
{
	char *names[] = { "Alice", "Bob", "Charlie", "Dave", 0 };

	char **filteredNames = filter(
			names,
			^(char *name){return strlen(name) >= 4;}
		);

	for (char **i = filteredNames; *i; i++){
		printf("%s\n", *i);
	}
}
