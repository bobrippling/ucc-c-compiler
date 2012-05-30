#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ucc_ext.h"
#include "ucc.h"
#include "../util/alloc.h"

#define TODO() printf("TODO: %s\n", __func__)

static char *
where()
{
	static char *w;

	if(!w){
		char *s;

		w = ustrdup(argv0);

		s = strrchr(w, '/');
		if(s)
			s[1] = '\0';
	}

	return w;
}

char *
actual_path(const char *prefix, const char *path)
{
	char *w = where();
	char *buf;

	buf = umalloc(strlen(w) + strlen(prefix) + strlen(path) + 2);

	sprintf(buf, "%s/%s%s", w, prefix, path);

	return buf;
}

void preproc(char *in, char *out, char **args)
{
	TODO();
}

void compile(char *in, char *out, char **args)
{
	TODO();
}

void assemble(char *in, char *out, char **args)
{
	TODO();
}

void link(char **objs, char *out, char **args)
{
	TODO();
}
