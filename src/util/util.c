#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "util.h"
#include "alloc.h"

const char *where_str(struct where *w)
{
	static char buf[128];
	snprintf(buf, sizeof buf, "%s:%d:%d", w->fname, w->line, w->chr + 1);
	return buf;
}

void vdie(struct where *w, va_list l, const char *fmt)
{
	if(w)
		fprintf(stderr, "%s: ", where_str(w));

	vfprintf(stderr, fmt, l);

	if(fmt[strlen(fmt)-1] == ':'){
		fputc(' ', stderr);
		perror(NULL);
	}else{
		fputc('\n', stderr);
	}

	exit(1);
}

void die_at(struct where *w, const char *fmt, ...)
{
	va_list l;
	struct where instead;

	if(!w){
		extern const char *current_fname;
		extern int current_line, current_chr;

		w = &instead;

		instead.fname = current_fname;
		instead.line  = current_line;
		instead.chr   = current_chr;
	}

	va_start(l, fmt);
	vdie(w, l, fmt);
	/* unreachable */
}

void die(const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	vdie(NULL, l, fmt);
	/* unreachable */
}

void die_ice(const char *fnam, int lin)
{
	fprintf(stderr, "ICE @ %s:%d\n", fnam, lin);
	exit(2);
}

char *fline(FILE *f)
{
	int c, pos = 0, len = 10;
	char *line = umalloc(len);

	do{
		if((c = fgetc(f)) == EOF){
			free(line);
			return NULL;
		}

		line[pos++] = c;
		if(pos == len){
			char *tmp;
			len *= 2;
			tmp = realloc(line, len);
			if(!tmp){
				free(line);
				return NULL;
			}
			line = tmp;
			line[pos+1] = '\0';
		}

		if(c == '\n'){
			line[pos-1] = '\0';
			return line;
		}
	}while(1);
}

void dynarray_add(void ***par, void *new)
{
	void **ar = *par;
	int idx = 0;

	if(!ar){
		ar = umalloc(2 * sizeof(void *));
	}else{
		int len = 0;
		while(ar[len])
			len++;
		idx = len;
		ar = urealloc(ar, (++len + 1) * sizeof(void *));
	}

	ar[idx] = new;
	ar[idx+1] = NULL;

	*par = ar;
}
