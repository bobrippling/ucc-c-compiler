#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "util.h"
#include "path.h"
#include "alloc.h"
#include "dynmap.h"

#define DIE() ice(__FILE__, __LINE__, __func__, NULL)

#define BAD(...) icw(__FILE__, __LINE__, __func__, __VA_ARGS__)

static int ec;

void ice(const char *f, int line, const char *fn, const char *fmt, ...)
{
	(void)fmt;
	fprintf(stderr, "%s:%d: ice: %s\n", f, line, fn);
	exit(1);
}

void icw(const char *f, int line, const char *fn, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	fprintf(stderr, "%s:%d: %s: ", f, line, fn);
	vfprintf(stderr, fmt, l);
	va_end(l);
	fputc('\n', stderr);

	ec = 1;
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

static void test_canon_all(void)
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

	TEST_CANON("/usr/include/hi", "/usr/include/hi");
	TEST_CANON("///usr/include/hi", "/usr/include/hi");
}

static int *new_int(int v)
{
	int *p = malloc(sizeof *p);
	*p = v;
	return p;
}

static void test_dynmap_normal(void)
{
	dynmap *map = dynmap_new((dynmap_cmp_f *)strcmp, dynmap_strhash);
	int i;
	char *key;
	int *removed;

	free(dynmap_set(char *, int *, map, (char *)"one", new_int(1)));
	free(dynmap_set(char *, int *, map, (char *)"two", new_int(2)));
	free(dynmap_set(char *, int *, map, (char *)"three", new_int(3)));
	free(dynmap_set(char *, int *, map, (char *)"hi", new_int(5)));
	free(dynmap_set(char *, int *, map, (char *)"hi", new_int(7)));

	for(i = 0; (key = dynmap_key(char *, map, i)); i++){
		int *val_ith = dynmap_value(int *, map, i);
		int *val_get = dynmap_get(char *, int *, map, key);
		int expected = -1;

		if(val_ith != val_get)
			BAD("dynmap_val != dynmap_get");

		switch(*key){
			case 'o':
				expected = 1;
				break;
			case 't':
				if(key[1] == 'w')
					expected = 2;
				else
					expected = 3;
				break;
			case 'h':
				expected = 7;
				break;
			default:
				BAD("bad key");
		}

		if(*val_ith != expected)
			BAD("dynmap_value(\"%s\") == %d", key, *val_ith);
	}

	if(i != 4) /* count */
		BAD("bad count (%d)", i);

	removed = dynmap_rm(char *, int *, map, (char *)"three");
	if(!removed || *removed != 3)
		BAD("removed != 3");
	for(i = 0; (key = dynmap_key(char *, map, i)); i++)
		;
	if(i != 3) /* count */
		BAD("bad count (%d)", i);

	while((key = dynmap_key(char *, map, 0)))
		free(dynmap_rm(char *, int *, map, key));

	dynmap_free(map);
}

static unsigned eq_hash(const void *p)
{
	(void)p;
	return 5;
}

static void test_dynmap_collision(void)
{
	dynmap *map = dynmap_new(/*ref*/NULL, eq_hash);
	int i;
	char *key;

	(void)dynmap_set(char *, char *, map, (char *)"hi", (char *)"formal");
	(void)dynmap_set(char *, char *, map, (char *)"yo", (char *)"informal");

	for(i = 0; (key = dynmap_key(char *, map, i)); i++)
		;

	if(i != 2)
		BAD("collision merged?");

	if(strcmp(dynmap_get(char *, char *, map, (char *)"hi"), "formal"))
		BAD("messed up dynmap entry");
	if(strcmp(dynmap_get(char *, char *, map, (char *)"yo"), "informal"))
		BAD("messed up dynmap entry");

	dynmap_free(map);
}

static void test_dynmap(void)
{
	test_dynmap_normal();
	test_dynmap_collision();
}

int main()
{
	test_dynmap();
	test_canon_all();

	return ec;
}
