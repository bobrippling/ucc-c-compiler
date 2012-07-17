#include <stdio.h>
#include <string.h>

run(const char *nam, FILE *f)
{
	printf("--- %s ---\n", nam);

	fprintf(f, "hello %s %d\n", "there", 5);

	fprintf(f, "at %d\n", ftell(f));

	fseek(f, -5, SEEK_END);
	rewind(f);

	{
		char buf[16], *p;

		if(fgets(buf, 16, f)){
			p = strchr(buf, '\n');
			if(p)
				*p = 0;

			printf("read \"%s\"\n", buf);
		}else{
			printf("no read\n");
		}
	}

	if(f != stdout)
		fclose(f);

	printf("--- fin ---\n");
}

do_read(void *cookie, char *p, int n)
{
	printf("%s, cookie=%p, p=%p, n=%d\n",
			__func__, cookie, p, n);

	strncpy(p, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", n);

	return n;
}

do_write(void *cookie, const char *p, int n)
{
	printf("%s, cookie=%p, p=%p='%s', n=%d\n",
			__func__, cookie, p, p, n);
}

do_seek(void *cookie, fpos_t p, int w)
{
	printf("%s, cookie=%p, pos=%d, whence=%d\n",
			__func__, cookie, p, w);

	return 32;
}

do_close(void *cookie)
{
	printf("%s, cookie=%p\n",
			__func__, cookie);
}

main()
{
	run("-", stdout);

	run("funopen", funopen((void *)5, do_read, &do_write, do_seek, do_close));

	run("fropen", fropen(NULL, do_read));

	run("fwopen", fwopen(NULL, do_write));
}
