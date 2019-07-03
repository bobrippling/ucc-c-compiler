#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "util.h"
#include "path.h"
#include "alloc.h"
#include "dynmap.h"
#include "dynarray.h"
#include "math.h"

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

static void test(int cond, const char *expr)
{
	if(!cond){
		ec = 1;
		fprintf(stderr, "test failed: %s\n", expr);
	}
}
#define test(exp) test((exp), #exp)

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
	TEST_CANON( \
				"./hello///there//..//tim/./file.", \
				"hello/tim/file.");

	TEST_CANON( \
				"./hello///there//..//tim/./file../.dir/", \
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
	dynmap *map = dynmap_new(char *, strcmp, dynmap_strhash);
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

static unsigned eq_hash(const char *p)
{
	(void)p;
	return 5;
}

static void test_dynmap_collision(void)
{
	dynmap *map = dynmap_new(char *, /*ref*/NULL, eq_hash);
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

static void test_dynarray(void)
{
	int **ints = NULL;

	dynarray_add(&ints, (int *)3);
	dynarray_add(&ints, (int *)2);
	dynarray_add(&ints, (int *)1);

	test(dynarray_count(ints) == 3);
	test(ints[0] == (int *)3);
	test(ints[1] == (int *)2);
	test(ints[2] == (int *)1);
	test(ints[3] == NULL);

	dynarray_rm(&ints, (int *)2);
	{
		test(dynarray_count(ints) == 2);
		test(ints[0] == (int *)3);
		test(ints[1] == (int *)1);
		test(ints[2] == NULL);
	}

	test(dynarray_pop(int *, &ints) == (int *)1);
	{
		test(dynarray_count(ints) == 1);
		test(ints[0] == (int *)3);
		test(ints[1] == NULL);
	}

	dynarray_prepend(&ints, (int *)9);
	{
		test(dynarray_count(ints) == 2);
		test(ints[0] == (int *)9);
		test(ints[1] == (int *)3);
		test(ints[2] == NULL);
	}

	dynarray_rm(&ints, (int *)3);
	{
		test(dynarray_count(ints) == 1);
		test(ints[0] == (int *)9);
		test(ints[1] == NULL);
	}

	dynarray_rm(&ints, (int *)9);
	{
		test(dynarray_count(ints) == 0);
		test(ints == NULL);
	}

	typedef struct A { int i; } A;
	A **as = NULL, *insert;
	int i;

	for(i = 0; i < 10; i++){
		A *a = umalloc(sizeof *a);
		a->i = i;
		dynarray_add(&as, a);
	}

	test(dynarray_count(as) == 10);

	test(as[3]->i == 3);
	test(as[10] == NULL);

	insert = umalloc(sizeof *insert);
	insert->i = 53;
	dynarray_insert(&as, 3, insert);

	test(dynarray_count(as) == 11);
	test(as[3]->i == 53);
	test(as[4]->i == 3);
	test(as[9]->i == 8);
	test(as[10]->i == 9);
	test(as[11] == NULL);

	insert = umalloc(sizeof *insert);
	insert->i = -1;
	dynarray_insert(&as, 0, insert);

	test(dynarray_count(as) == 12);
	test(as[0]->i == -1);
	test(as[1]->i == 0);
	test(as[2]->i == 1);
	test(as[4]->i == 53);
	test(as[5]->i == 3);
	test(as[10]->i == 8);
	test(as[11]->i == 9);
	test(as[12] == NULL);

	dynarray_free(A **, as, free);
	test(as == NULL);
}

static void test_math(void)
{
	/* 0b1011010 */
	unsigned x = 0x5a;
	test(extractbottombit(&x) == 0x2);
	test(x == 0x58);

	test(log2i(extractbottombit(&x)) == 3);
	test(x == 0x50);
}

int main(void)
{
	test_dynmap();
	test_dynarray();
	test_canon_all();
	test_math();

	return ec;
}
