#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/where.h"
#include "cc1.h"
#include "cc1_where.h"
#include "out/asm.h"
#include "fopt.h"
#include "funcargs.h"

enum cc1_backend cc1_backend = BACKEND_ASM;
int cc1_error_limit = 16;
char *cc1_first_fname;
enum debug_level cc1_gdebug = DEBUG_OFF;
int cc1_gdebug_columninfo;
int cc1_mstack_align;
enum c_std cc1_std = STD_C99;
struct cc1_warning cc1_warning;
FILE *cc_out[NUM_SECTIONS];     /* temporary section files */
struct cc1_fopt cc1_fopt;
enum mopt mopt_mode;
struct section sections[NUM_SECTIONS];
int show_current_line;
enum san_opts cc1_sanitize;
char *cc1_sanitize_handler_fn;
enum visibility cc1_visibility_default;

int where_in_sysheader(const where *w)
{
	(void)w;
	return 1;
}

/* ------------ */

#include "type_nav.h"

static int ec;

static void test(int cond, const char *expr, int line)
{
	if(!cond){
		ec = 1;
		fprintf(stderr, "%s:%d: test failed: %s\n", __FILE__, line, expr);
	}
}
#define test(exp) test((exp), #exp, __LINE__)

static void test_quals(void)
{
	type *tint = type_nav_btype(cc1_type_nav, type_int);
	type *tconstint = type_qualify(tint, qual_const);

	test(tconstint == type_qualify(tconstint, qual_const));
}

static void test_decl_interposability(void)
{
	sym s = { 0 };
	decl d = { 0 };
	decl d_fn = { 0 };
	decl d_extern = { 0 };
	decl d_extern_fn = { 0 };
	struct type_nav *types = type_nav_init();
	funcargs args = { 0 };

	s.type = sym_global;

	d.sym = &s;
	d_fn.sym = &s;
	d_extern.sym = &s;
	d_extern_fn.sym = &s;

	d.ref = type_nav_btype(types, type_int);
	d_fn.ref = type_func_of(type_nav_btype(types, type_int), &args, NULL);
	d_extern.ref = type_nav_btype(types, type_int);
	d_extern_fn.ref = type_func_of(type_nav_btype(types, type_int), &args, NULL);

	d_fn.bits.func.code = (void *)1;
	d_extern.store = store_extern;

	cc1_fopt.pic = 0;
	cc1_fopt.pie = 0;
	cc1_fopt.semantic_interposition = 1;

	test(decl_visibility(&d) == VISIBILITY_DEFAULT);
	test(decl_linkage(&d) == linkage_external);
	test(!decl_interposable(&d));

	d.store = store_static;
	d_fn.store = store_static;
	{
		test(decl_visibility(&d) == VISIBILITY_DEFAULT);
		test(decl_linkage(&d) == linkage_internal);
		test(!decl_interposable(&d));

		test(decl_visibility(&d_fn) == VISIBILITY_DEFAULT);
		test(decl_linkage(&d_fn) == linkage_internal);
		test(!decl_interposable(&d_fn));
	}
	d.store = store_default;
	d_fn.store = store_default;

	cc1_visibility_default = VISIBILITY_PROTECTED;
	{
		test(decl_visibility(&d) == VISIBILITY_PROTECTED);
		test(decl_linkage(&d) == linkage_external);
		test(!decl_interposable(&d));

		test(decl_visibility(&d_fn) == VISIBILITY_PROTECTED);
		test(!decl_interposable(&d_fn));

		test(decl_visibility(&d_extern) == VISIBILITY_DEFAULT);
		test(!decl_interposable(&d_extern));

		test(decl_visibility(&d_extern_fn) == VISIBILITY_DEFAULT);
		test(!decl_interposable(&d_extern_fn));
	}
	cc1_visibility_default = VISIBILITY_DEFAULT;

	cc1_visibility_default = VISIBILITY_PROTECTED;
	cc1_fopt.pic = 1;
	{
		test(decl_visibility(&d) == VISIBILITY_PROTECTED);
		test(decl_linkage(&d) == linkage_external);
		test(!decl_interposable(&d));

		test(decl_visibility(&d_fn) == VISIBILITY_PROTECTED);
		test(!decl_interposable(&d_fn));

		test(decl_visibility(&d_extern) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_extern));

		test(decl_visibility(&d_extern_fn) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_extern_fn));
	}
	cc1_visibility_default = VISIBILITY_DEFAULT;
	cc1_fopt.pic = 0;

	cc1_fopt.pic = 1;
	{
		test(decl_visibility(&d) == VISIBILITY_DEFAULT);
		test(decl_linkage(&d) == linkage_external);
		test(decl_interposable(&d));

		test(decl_visibility(&d_fn) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_fn));

		test(decl_visibility(&d_extern) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_extern));

		test(decl_visibility(&d_extern_fn) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_extern_fn));
	}
	cc1_fopt.pic = 0;

	cc1_visibility_default = VISIBILITY_HIDDEN;
	cc1_fopt.pic = 1;
	{
		test(decl_visibility(&d) == VISIBILITY_HIDDEN);
		test(decl_linkage(&d) == linkage_external);
		test(!decl_interposable(&d));

		test(decl_visibility(&d_fn) == VISIBILITY_HIDDEN);
		test(!decl_interposable(&d_fn));

		test(decl_visibility(&d_extern) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_extern));

		test(decl_visibility(&d_extern_fn) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_extern_fn));
	}
	cc1_fopt.pic = 0;
	cc1_visibility_default = VISIBILITY_DEFAULT;

	cc1_fopt.pie = 1;
	{
		test(decl_visibility(&d) == VISIBILITY_DEFAULT);
		test(decl_linkage(&d) == linkage_external);
		test(!decl_interposable(&d));

		test(decl_visibility(&d_fn) == VISIBILITY_DEFAULT);
		test(!decl_interposable(&d_fn));

		test(decl_visibility(&d_extern) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_extern));

		test(decl_visibility(&d_extern_fn) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_extern_fn));
	}
	cc1_fopt.pie = 0;

	cc1_fopt.pic = 1;
	cc1_fopt.semantic_interposition = 0;
	{
		test(decl_visibility(&d) == VISIBILITY_DEFAULT);
		test(decl_linkage(&d) == linkage_external);
		test(!decl_interposable(&d));

		test(decl_visibility(&d_fn) == VISIBILITY_DEFAULT);
		test(!decl_interposable(&d_fn));

		test(decl_visibility(&d_extern) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_extern));

		test(decl_visibility(&d_extern_fn) == VISIBILITY_DEFAULT);
		test(decl_interposable(&d_extern_fn));
	}
	cc1_fopt.pic = 0;
	cc1_fopt.semantic_interposition = 1;
}

static void test_decl_needs_GOTPLT(void)
{
	sym s;
	struct type_nav *types = type_nav_init();
	funcargs args = { 0 };
	decl d_extern = { 0 };
	decl d_normal = { 0 };
	decl d_fn_undef = { 0 };
	decl d_fn_defined = { 0 };
	decl d_protected = { 0 };
	decl d_fn_protected = { 0 };
	attribute attr_protected = { 0 };
	attribute *attr[] = {
		&attr_protected, NULL
	};

	attr_protected.type = attr_visibility;
	attr_protected.bits.visibility = VISIBILITY_PROTECTED;

	d_extern.sym = &s;
	d_normal.sym = &s;
	d_fn_undef.sym = &s;
	d_fn_defined.sym = &s;
	d_protected.sym = &s;
	d_fn_protected.sym = &s;

	s.type = sym_global;

	/* extern int d_extern;
	 * int d_normal;
	 * void fn_undef(void);
	 * void fn_defined(void){}
	 */
	d_extern.ref = type_nav_btype(types, type_int);
	d_normal.ref = type_nav_btype(types, type_int);
	d_fn_undef.ref = type_func_of(type_nav_btype(types, type_int), &args, NULL);
	d_fn_defined.ref = type_func_of(type_nav_btype(types, type_int), &args, NULL);
	d_protected.ref = type_nav_btype(types, type_int);
	d_fn_protected.ref = type_func_of(type_nav_btype(types, type_int), &args, NULL);

	d_extern.store = store_extern;
	d_fn_defined.bits.func.code = (void *)1;
	d_protected.attr = attr;
	d_fn_protected.attr = attr;

	cc1_fopt.pic = 0;
	cc1_fopt.pie = 0;
	{
		test(!decl_needs_GOTPLT(&d_extern));
		test(!decl_needs_GOTPLT(&d_normal));
		test(!decl_needs_GOTPLT(&d_fn_undef));
		test(!decl_needs_GOTPLT(&d_fn_defined));
		test(!decl_needs_GOTPLT(&d_protected));
		test(!decl_needs_GOTPLT(&d_fn_protected));
	}

	cc1_fopt.pic = 1;
	{
		test(decl_needs_GOTPLT(&d_extern));
		test(decl_needs_GOTPLT(&d_normal));
		test(decl_needs_GOTPLT(&d_fn_undef));
		test(decl_needs_GOTPLT(&d_fn_defined));
		test(!decl_needs_GOTPLT(&d_protected));
		test(!decl_needs_GOTPLT(&d_fn_protected));
	}
	cc1_fopt.pic = 0;

	cc1_fopt.pie = 1;
	{
		test(decl_needs_GOTPLT(&d_extern));
		test(!decl_needs_GOTPLT(&d_normal));
		test(decl_needs_GOTPLT(&d_fn_undef));
		test(!decl_needs_GOTPLT(&d_fn_defined));
		test(!decl_needs_GOTPLT(&d_protected));
		test(!decl_needs_GOTPLT(&d_fn_protected));
	}
	cc1_fopt.pie = 0;

	cc1_fopt.pic = 1;
	cc1_visibility_default = VISIBILITY_HIDDEN;
	{
		test(decl_needs_GOTPLT(&d_extern));
		test(!decl_needs_GOTPLT(&d_normal));
		test(decl_needs_GOTPLT(&d_fn_undef));
		test(!decl_needs_GOTPLT(&d_fn_defined));
		test(!decl_needs_GOTPLT(&d_protected));
		test(!decl_needs_GOTPLT(&d_fn_protected));
	}
	cc1_fopt.pic = 0;
	cc1_visibility_default = VISIBILITY_DEFAULT;
}

int main(void)
{
	cc1_type_nav = type_nav_init();

	test_quals();
	test_decl_interposability();
	test_decl_needs_GOTPLT();

	return ec;
}
