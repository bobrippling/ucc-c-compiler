#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/where.h"
#include "cc1.h"
#include "cc1_where.h"
#include "out/asm.h"

#include "op.h"
#include "sym.h"
#include "gen_ir.h"

#define countof(x) (sizeof(x) / sizeof((x)[0]))

enum cc1_backend cc1_backend = BACKEND_ASM;
int cc1_error_limit = 16;
char *cc1_first_fname;
int cc1_gdebug;
int cc1_mstack_align;
enum c_std cc1_std = STD_C99;
struct cc1_warning cc1_warning;
FILE *cc_out[NUM_SECTIONS];     /* temporary section files */
enum fopt fopt_mode;
enum mopt mopt_mode;
struct section sections[NUM_SECTIONS];
int show_current_line;
enum san_opts cc1_sanitize;
char *cc1_sanitize_handler_fn;

int cc1_warn_at_w(
		const struct where *where, unsigned char *pwarn,
		const char *fmt, ...)
{
	(void)where;
	(void)pwarn;
	(void)fmt;
	return 0;
}

int where_in_sysheader(const where *w)
{
	(void)w;
	return 1;
}

/* ------------ */

#include "type_nav.h"

static int ec;

static void test(int cond, const char *expr, const char *file, int line)
{
	if(!cond){
		ec = 1;
		fprintf(stderr, "%s:%d: test failed: %s\n", file, line, expr);
	}
}
#define test(exp) test((exp), #exp, __FILE__, __LINE__)

static void test_quals(void)
{
	type *tint = type_nav_btype(cc1_type_nav, type_int);
	type *tconstint = type_qualify(tint, qual_const);

	test(tconstint == type_qualify(tconstint, qual_const));
}

static void test_ir_nonbitfield_enumeration(void)
{
	decl ds[3] = { 0 };
	sue_member members[2] = { &ds[0], &ds[1] };
	sue_member *pmembers[3] = { &members[0], &members[1], NULL };
	struct_union_enum_st su = { 0 };
	unsigned idx;

	su.members = pmembers;

	test(irtype_struct_decl_index(&su, &ds[0], &idx));
	test(idx == 0);

	test(irtype_struct_decl_index(&su, &ds[1], &idx));
	test(idx == 1);

	test(!irtype_struct_decl_index(&su, &ds[2], &idx));
}

static expr *folded_val(int v)
{
	expr *e = expr_new_val(v);
	e->tree_type = type_nav_btype(cc1_type_nav, type_int);
	return e;
}

static void test_ir_bitfield_enumeration(void)
{
	decl ds[8] = { 0 };
	decl not_in = { 0 };
	sue_member members[countof(ds)];
	sue_member *pmembers[countof(members) + 1];
	struct_union_enum_st su = { 0 };
	unsigned idx;

	for(idx = 0; idx < countof(ds); idx++){
		members[idx].struct_member = &ds[idx];
		pmembers[idx] = &members[idx];
	}
	pmembers[idx] = NULL;

	ds[0].bits.var.field_width = folded_val(5);
	ds[0].bits.var.first_bitfield = 1;

	ds[1].bits.var.field_width = folded_val(2);

	/* test zero width: */
	ds[2].bits.var.field_width = folded_val(0);
	ds[2].bits.var.first_bitfield = 1;

	ds[3].bits.var.field_width = folded_val(3);
	ds[3].bits.var.first_bitfield = 1;

	/* test overflow */
	ds[4].bits.var.field_width = folded_val(26);

	ds[5].bits.var.field_width = folded_val(1);
	ds[5].bits.var.first_bitfield = 1; /* set by fold_sue - overflow handled */

	ds[6].bits.var.field_width = NULL;

	ds[7].bits.var.field_width = folded_val(2);
	ds[7].bits.var.first_bitfield = 1;

	su.members = pmembers;

	test(!irtype_struct_decl_index(&su, &not_in, &idx));

	test(irtype_struct_decl_index(&su, &ds[0], &idx));
	test(idx == 0);

	test(irtype_struct_decl_index(&su, &ds[1], &idx));
	test(idx == 0);

	test(irtype_struct_decl_index(&su, &ds[3], &idx));
	test(idx == 1);

	test(irtype_struct_decl_index(&su, &ds[4], &idx));
	test(idx == 1);

	test(irtype_struct_decl_index(&su, &ds[5], &idx));
	test(idx == 2);

	test(irtype_struct_decl_index(&su, &ds[6], &idx));
	test(idx == 3);

	test(irtype_struct_decl_index(&su, &ds[7], &idx));
	test(idx == 4);
}

int main(void)
{
	cc1_type_nav = type_nav_init();

	test_quals();

	test_ir_bitfield_enumeration();
	test_ir_nonbitfield_enumeration();

	return ec;
}
