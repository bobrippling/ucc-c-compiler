#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "util.h"
#include "path.h"
#include "alloc.h"

#define DIE() ice(__FILE__, __LINE__, __func__, NULL)

void ice(const char *f, int line, const char *fn, const char *fmt, ...)
{
	(void)fmt;
	fprintf(stderr, "ice: %s:%d: %s\n", f, line, fn);
	exit(1);
}

int main()
{
	if(strcmp(
			canonicalise_path(ustrdup("./hello///there//..//tim/./file.")),
			"hello/tim/file."))
	{
		DIE();
	}

	if(strcmp(
			canonicalise_path(ustrdup("./hello///there//..//tim/./file../.dir/")),
			"hello/tim/file../.dir/"))
	{
		DIE();
	}

	if(strcmp(canonicalise_path(ustrdup("../")), "../"))
		DIE();

	if(strcmp(canonicalise_path(ustrdup("..")), ".."))
		DIE();

	if(strcmp(canonicalise_path(ustrdup("./..")), ".."))
		DIE();

	if(strcmp(canonicalise_path(ustrdup("../..")), "../.."))
		DIE();

	if(strcmp(canonicalise_path(ustrdup("./../../")), "../../"))
		DIE();

	if(strcmp(canonicalise_path(ustrdup("../../hi")), "../../hi"))
		DIE();

	if(strcmp(canonicalise_path(ustrdup("hi/../../")), "../"))
		DIE();

	return 0;
}
