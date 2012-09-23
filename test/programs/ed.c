#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

typedef struct line
{
	char *line;
	struct line *next;
} fline;

typedef struct
{
	fline *head;
	const char *fname;
} fbuf;

char *strdup(const char *s)
{
	char *r = malloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}

void die(const char *fmt, ...)
{
	va_list l;
	int len;

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);

	len = strlen(fmt);
	if(len > 0 && fmt[len-1] == ':')
		fprintf(stderr, " %s\n", strerror(errno));

	exit(1);
}

fbuf *fbufopen(const char *nam)
{
	FILE *f = fopen(nam, "r");
	char buf[256];
	fbuf *ret;
	fline **ptail;

	if(!f)
		die("fopen %s:", nam);

	ret = malloc(sizeof *ret);
	memset(ret, 0, sizeof *ret);
	ptail = &ret->head;

	while(fgets(buf, sizeof buf, f)){
		fline *tail = malloc(sizeof *tail);
		tail->line = strdup(buf);
		tail->next = NULL;
		*ptail = tail;
		ptail = &tail->next;
	}

	fclose(f);

	return ret;
}

int main(int argc, char **argv)
{
	fbuf *f;
	fline *l;

	if(argc != 2){
		fprintf(stderr, "Usage: %s file\n", *argv);
		return 1;
	}

	f = fbufopen(argv[1]);

	for(l = f->head; l; l = l->next)
		printf("line \"%s\"\n", l->line);

	return 0;
}
