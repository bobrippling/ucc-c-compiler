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

void vwarn(struct where *w, const char *fmt, va_list l)
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

void vdie(struct where *w, const char *fmt, va_list l)
{
	vwarn(w, fmt, l);
	exit(1);
}

void warn_at(struct where *w, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vwarn(w, fmt, l);
	/* unreachable */
}

void die_at(struct where *w, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vdie(w, fmt, l);
	/* unreachable */
}

void die(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vdie(NULL, fmt, l); /* FIXME: this is called before current_fname etc is init'd */
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
			len *= 2;
			line = urealloc(line, len);
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

	UCC_ASSERT(new, "dynarray_add(): adding NULL");

	if(!ar){
		ar = umalloc(2 * sizeof *ar);
	}else{
		idx = dynarray_count(ar);
		ar = urealloc(ar, (idx + 2) * sizeof *ar);
	}

	ar[idx] = new;
	ar[idx+1] = NULL;

	*par = ar;
}

void dynarray_prepend(void ***par, void *new)
{
	void **ar;
	int i;

	dynarray_add(par, new);

	ar = *par;

//#define SLOW
#ifdef SLOW
	for(i = dynarray_count(ar) - 2; i >= 0; i--)
		ar[i + 1] = ar[i];
#else
	i = dynarray_count(ar) - 1;
	if(i > 0)
		memmove(ar + 1, ar, i * sizeof *ar);
#endif

	ar[0] = new;
}

int dynarray_count(void **ar)
{
	int len = 0;

	if(!ar)
		return 0;

	while(ar[len])
		len++;

	return len;
}

void dynarray_free(void ***par, void (*f)(void *))
{
	void **ar = *par;

	if(ar){
		while(*ar){
			f(*ar);
			ar++;
		}
		free(*par);
		*par = NULL;
	}
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

char *ext_replace(const char *str, const char *ext)
{
	char *dot;
	dot = strrchr(str, '.');

	if(dot){
		char *dup;

		dup = umalloc(strlen(ext) + strlen(str));

		strcpy(dup, str);
		sprintf(dup + (dot - str), ".%s", ext);

		return dup;
	}else{
		return ustrdup(str);
	}
}
