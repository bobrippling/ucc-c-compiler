#include <stdio.h>

run(FILE *f)
{
	fprintf(f, "hello %s %d\n", "there", 5);
}

do_write(void *cookie, const char *p, int n)
{
	printf("%s, cookie=%p, p=%p='%s', n=%d\n",
			__func__, cookie, p, p, n);
}

do_close(void *cookie)
{
	printf("%s, cookie=%p\n",
			__func__, cookie);
}

main()
{
	FILE *f;

	run(stdout);

	f = funopen((void *)5, NULL, &do_write, NULL, do_close);

	run(f);

	return fclose(f);
}
