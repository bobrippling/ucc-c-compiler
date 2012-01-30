#define READ  0
#define WRITE 1

#define USE_MALLOC
#define SYS_pipe 22

int pipe_manual(int x[2])
{
	return __syscall(SYS_pipe, x);
}

main()
{
#ifdef USE_MALLOC
	int *p;
#else
	int p[2];
#endif
	int r;

#ifdef USE_MALLOC
	p = malloc(2 * sizeof *p);
#endif

	printf("pipe(p) = %d\n", r = pipe(p));

	if(r)
		return 1;

	write(p[WRITE], "hi\n", 3);
	close(p[WRITE]);

	while(read(p[READ], &r, 1) > 0)
		putchar(r);

	close(p[READ]);

#ifdef USE_MALLOC
	free(p);
#endif

	return 0;
}
