#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/where.h"
#include "../util/macros.h"
#include "../util/warn.h"
#include "../util/alloc.h"

#include "warn.h"
#include "fopt.h"

/* int *_had_error */
#include "fold.h"
#include "tokenise.h"

#include "cc1_where.h"
#include "cc1.h"


struct cc1_warning cc1_warning;

enum warning_special
{
	W_ALL, W_EXTRA, W_EVERYTHING, W_GNU
};

static struct warn_str
{
	const char *arg;
	unsigned char *offset;
} warns[] = {
#define X(arg, name) { arg, &cc1_warning.name },
#define ALIAS X
#define GROUP(arg, ...)
#include "warnings.def"
#undef X
#undef ALIAS
#undef GROUP
	{ NULL, NULL }
};

static struct warn_str_group
{
	const char *arg;
	unsigned char *offsets[3];
} warn_groups[] = {
#define X(arg, name)
#define ALIAS X
#define GROUP(arg, ...) { arg, __VA_ARGS__ },
#include "warnings.def"
#undef X
#undef ALIAS
#undef GROUP
	{ NULL, NULL }
};

static void show_warn_option(
		const unsigned char *poffset,
		const unsigned char *pwarn,
		const char *arg)
{
	if(pwarn != poffset)
		return;

	fprintf(stderr, "[-W%s]\n", arg);
}

static void show_warn_options(const unsigned char *pwarn)
{
	struct warn_str *p;
	struct warn_str_group *p_group;

	for(p = warns; p->arg; p++)
		show_warn_option(p->offset, pwarn, p->arg);

	for(p_group = warn_groups; p_group->arg; p_group++){
		unsigned i;

		for(i = 0; i < countof(p_group->offsets) && p_group->offsets[i]; i++){
			show_warn_option(p_group->offsets[i], pwarn, p_group->arg);
		}
	}
}

int cc1_warn_at_w(
		const struct where *where, const unsigned char *pwarn,
		const char *fmt, ...)
{
	va_list l;
	struct where backup;
	enum warn_type warn_type = VWARN_WARN;

	switch((enum warning_fatality)*pwarn){
		case W_OFF:
			return 0;
		case W_ERROR:
			fold_had_error = parse_had_error = 1;
			warn_type = VWARN_ERR;
			break;
		case W_NO_ERROR:
		case W_WARN:
			break;
	}

	if(!where)
		where = where_cc1_current(&backup);

	/* don't emit warnings from system headers */
	if(!cc1_warning.system_headers && where_in_sysheader(where)){
		if(warn_type == VWARN_ERR){
			/* we always want this warning emitting */
		}else{
			return 0;
		}
	}

	va_start(l, fmt);
	vwarn(where, warn_type, fmt, l);
	va_end(l);

	if(cc1_fopt.show_warning_option)
		show_warn_options(pwarn);

	return 1;
}

void warnings_set(enum warning_fatality to)
{
	memset(&cc1_warning, to, sizeof cc1_warning);
}

static void warning_gnu(enum warning_fatality set)
{
	cc1_warning.gnu_addr_lbl =
	cc1_warning.gnu_expr_stmt =
	cc1_warning.gnu_typeof =
	cc1_warning.gnu_autotype =
	cc1_warning.gnu_attribute =
	cc1_warning.gnu_init_array_range =
	cc1_warning.gnu_case_range =
	cc1_warning.gnu_alignof_expr =
	cc1_warning.gnu_empty_init =
		set;
}

void warning_pedantic(enum warning_fatality set)
{
	/* warn about extensions */
	cc1_warning.nonstd_arraysz =
	cc1_warning.nonstd_init =

	cc1_warning.x__func__init =
	cc1_warning.typedef_fnimpl =
	cc1_warning.flexarr_only =
	cc1_warning.decl_nodecl =
	cc1_warning.overlarge_enumerator_int =

	cc1_warning.attr_printf_voidp =

	cc1_warning.return_void =
	cc1_warning.binary_literal =

	cc1_warning.overlength_strings =
		set;
}

static void warning_all(enum warning_fatality set)
{
	warnings_set(set);

	warning_gnu(W_OFF);

	cc1_warning.implicit_int =
	cc1_warning.truncation =
	cc1_warning.pad =
	cc1_warning.tenative_init =
	cc1_warning.shadow_global_user =
	cc1_warning.shadow_global_sysheaders =
	cc1_warning.shadow_compatible_local =
	cc1_warning.implicit_old_func =
	cc1_warning.bitfield_boundary =
	cc1_warning.vla =
	cc1_warning.init_missing_struct_zero =
	cc1_warning.pure_inline =
	cc1_warning.unused_param =
	cc1_warning.test_assign =
	cc1_warning.sign_compare =
	cc1_warning.unused_fnret =
	cc1_warning.binary_literal =
	cc1_warning.missing_prototype =
	cc1_warning.missing_variable_decls =
	cc1_warning.null_zero_literal =
	cc1_warning.enum_out_of_range =
	cc1_warning.enum_mismatch_int =
	cc1_warning.inline_failed =
	cc1_warning.switch_default =
	cc1_warning.switch_default_covered =
	cc1_warning.sym_never_read =
	cc1_warning.system_headers =
	cc1_warning.int_op_promotion =
	cc1_warning.overlength_strings =
	cc1_warning.aggregate_return =
	cc1_warning.switch_enum_even_when_default_lbl =
	cc1_warning.bitfield_promotion =
		W_OFF;
}

void warning_init(void)
{
	/* default to -Wall */
	warning_all(W_WARN);
	warning_pedantic(W_OFF);

	/* but with warnings about std compatability on too */
	cc1_warning.typedef_redef =
	cc1_warning.c89_parse_trailingcomma =
	cc1_warning.unnamed_struct_memb_ext_tagged =
	cc1_warning.unnamed_struct_memb_ignored =
	cc1_warning.unnamed_struct_memb_ext_c11 =
	cc1_warning.c89_for_init =
	cc1_warning.mixed_code_decls =
	cc1_warning.c89_init_constexpr =
	cc1_warning.long_long =
	cc1_warning.c89_vla =
	cc1_warning.c89_compound_literal =
			W_WARN;

	/* also turn off global-unused warnings */
	cc1_warning.unused_param = W_OFF;
	cc1_warning.unused_var = W_OFF;
	cc1_warning.unused_function = W_OFF;

	/* but disable others */
	cc1_warning.char_subscript = W_OFF;

	/* mostly harmless type warnings */
	cc1_warning.int_op_promotion = W_OFF;
	cc1_warning.sign_compare = W_OFF;
}

static void warning_special(enum warning_special special, enum warning_fatality fatality)
{
	switch(special){
		case W_EVERYTHING:
			warnings_set(fatality);
			break;
		case W_ALL:
			warning_all(fatality);
			break;
		case W_EXTRA:
			warning_all(fatality);
			cc1_warning.implicit_int =
			cc1_warning.cast_qual =
			cc1_warning.init_missing_braces =
			cc1_warning.init_missing_struct =
			cc1_warning.unused_param =
				fatality;
			break;
		case W_GNU:
			warning_gnu(fatality);
			break;
	}
}

static void warning_unknown(const char *warg, dynmap *unknowns)
{
	char *dup_warg = ustrdup(warg);

	int present = dynmap_set(
			char *, intptr_t,
			unknowns, dup_warg, (intptr_t)1);

	if(present)
		free(dup_warg);
}

void warning_on(
		const char *warn, enum warning_fatality to,
		int *const werror, dynmap *unknowns)
{
	struct warn_str *p;
	struct warn_str_group *p_group;
	int found;

#define SPECIAL(str, w)   \
	if(!strcmp(warn, str)){ \
		warning_special(w, to); \
		return;               \
	}

	SPECIAL("all", W_ALL)
	SPECIAL("extra", W_EXTRA)
	SPECIAL("everything", W_EVERYTHING)
	SPECIAL("gnu", W_GNU)

	if(!strncmp(warn, "error", 5)){
		const char *werr = warn + 5;

		switch(*werr){
			case '\0':
				/* set later once we know all the desired warnings */
				*werror = (to != W_OFF);
				break;

			case '=':
				if(to == W_OFF){
					/* force to non-error */
					warning_on(werr + 1, W_NO_ERROR, werror, unknowns);
				}else{
					warning_on(werr + 1, W_ERROR, werror, unknowns);
				}
				break;

			default:
				warning_unknown(warn, unknowns);
		}
		return;
	}


	found = 0;
	for(p = warns; p->arg; p++){
		if(!strcmp(warn, p->arg)){
			*p->offset = to;
			found = 1;
			/* keep going for more aliases */
		}
	}
	if(found)
		return;

	for(p_group = warn_groups; p_group->arg; p_group++){
		if(!strcmp(warn, p_group->arg)){
			unsigned i;

			for(i = 0; i < countof(p_group->offsets) && p_group->offsets[i]; i++)
				*p_group->offsets[i] = to;
			return;
		}
	}

	warning_unknown(warn, unknowns);
}

void warnings_upgrade(void)
{
	struct warn_str *p;

	/* easier to iterate through this array, than cc1_warning's members */
	for(p = warns; p->arg; p++)
		if(*p->offset == W_WARN)
			*p->offset = W_ERROR;
}

int warnings_check_unknown(dynmap *unknown_warnings)
{
	where loc = { 0 };
	int hard_error = 0, got_unknown = 0;
	char *key;
	size_t i;

	loc.fname = "<command line>";

	switch(cc1_warning.unknown_warning_option){
		case W_OFF:
			return 0;
		case W_WARN:
		case W_NO_ERROR:
			break;
		case W_ERROR:
			hard_error = 1;
			break;
	}

	for(i = 0; (key = dynmap_key(char *, unknown_warnings, i)); i++){
		cc1_warn_at(&loc, unknown_warning_option,
				"unknown warning option: \"%s\"", key);
		got_unknown = 1;
	}

	return hard_error && got_unknown;
}
