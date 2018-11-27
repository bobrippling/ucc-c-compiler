#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "str.h"
#include "../util/util.h"

void ice(const char *f, int line, const char *fn, const char *fmt, ...)
{
	(void)f;
	(void)line;
	(void)fn;
	(void)fmt;
	abort();
}

static void test_rtrim(void)
{
	char buf[32] = { 0 };

	strcpy(buf, "abc123   ");
	rtrim(buf);
	assert(!strcmp(buf, "abc123"));

	strcpy(buf, "   ");
	rtrim(buf);
	assert(!strcmp(buf, ""));

	strcpy(buf, "");
	rtrim(buf);
	assert(!strcmp(buf, ""));
}

int main(void)
{
	test_rtrim();

	return 0;
}
