#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "util.h"
#include "alloc.h"

const char *where_str(const struct where *w)
{
	static char buf[128];
	snprintf(buf, sizeof buf, "%s:%d:%d", w->fname, w->line, w->chr + 1);
	return buf;
}

void vwarn(struct where *w, va_list l, const char *fmt)
{
	struct where instead;

	if(!w){
		extern const char *current_fname;
		extern int current_line, current_chr;

		w = &instead;

		instead.fname = current_fname;
		instead.line  = current_line;
		instead.chr   = current_chr;
	}

	fprintf(stderr, "%s: ", where_str(w));
	vfprintf(stderr, fmt, l);

	if(fmt[strlen(fmt)-1] == ':'){
		fputc(' ', stderr);
		perror(NULL);
	}else{
		fputc('\n', stderr);
	}
}

void vdie(struct where *w, va_list l, const char *fmt)
{
	vwarn(w, l, fmt);
	exit(1);
}

void warn_at(struct where *w, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vwarn(w, l, fmt);
	/* unreachable */
}

void die_at(struct where *w, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vdie(w, l, fmt);
	/* unreachable */
}

void die(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vdie(NULL, l, fmt); /* FIXME: this is called before current_fname etc is init'd */
	/* unreachable */
}

#define ICE_STR(s)  \
	va_list l; \
	fprintf(stderr, s " @ %s:%d: ", f, line); \
	va_start(l, fmt); \
	vfprintf(stderr, fmt, l); \
	fputc('\n', stderr)

void ice(const char *f, int line, const char *fmt, ...)
{
	ICE_STR("ICE");
	abort();
}

void icw(const char *f, int line, const char *fmt, ...)
{
	ICE_STR("ICW");
}

char *fline(FILE *f)
{
	int c, pos = 0, len = 10;
	char *line = umalloc(len);

	do{
		errno = 0;
		if((c = fgetc(f)) == EOF){
			if(errno == EINTR)
				continue;
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

char *udirname(const char *f)
{
	const char *fin;

	fin = strrchr(f, '/');

	if(fin){
		char *dup = ustrdup(f);
		dup[fin - f] = '\0';
		return dup;
	}else{
		return ustrdup("./");
	}
}
