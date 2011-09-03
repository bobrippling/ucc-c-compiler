void exit(int);
int  main(int, char **);

void __libc_main()
{
	int argc;
	char **argv;
	void *p;

	p = &argc - 2 * sizeof int;

	argc = *p;

	argv = p + sizeof int;

	/* TODO: environ */

	exit(main(argc, argv));
}

void _start()
{
	__libc_main(); /* set up stack pointers	*/
}
