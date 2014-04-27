#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "util.h"
#include "path.h"
#include "alloc.h"

#define DIE() ice(__FILE__, __LINE__, __func__, NULL)

static int ec;

void ice(const char *f, int line, const char *fn, const char *fmt, ...)
{
	(void)fmt;
	fprintf(stderr, "%s:%d: ice: %s\n", f, line, fn);
	exit(1);
}

static void test_canon(char *in, char *exp, int ln)
{
	char *dup = ustrdup(in);
	if(strcmp(canonicalise_path(dup), exp)){
		fprintf(stderr, "%s:%d: canon(\"%s\") = \"%s\", expected \"%s\"\n",
				__FILE__, ln, in, dup, exp);
		ec = 1;
	}
	free(dup);
}
#define TEST_CANON(in, exp) test_canon(in, exp, __LINE__)

int main()
{
	TEST_CANON(
				"./hello///there//..//tim/./file.",
				"hello/tim/file.");

	TEST_CANON(
				"./hello///there//..//tim/./file../.dir/",
				"hello/tim/file../.dir/");

	TEST_CANON("../", "../");
	TEST_CANON("..", "..");
	TEST_CANON("./..", "..");
	TEST_CANON("../..", "../..");
	TEST_CANON("./../../", "../../");
	TEST_CANON("../../hi", "../../hi");
	TEST_CANON("hi/../../", "../");

	TEST_CANON("../../hi/../..//../", "../../../../");

	return ec;
}
