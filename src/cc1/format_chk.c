#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "../util/where.h"
#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/platform.h"

#include "expr.h"
#include "decl.h"
#include "const.h"
#include "funcargs.h"
#include "type_is.h"
#include "warn.h"
#include "str.h"
#include "type_nav.h"

#include "format_chk.h"

#include "ops/expr_if.h"

#define DEBUG_FORMAT_CHECK 0

struct format_arg
{
	type *expected_type; /* NULL for "%%", "%m" */
	int expected_field_width;
	int expected_precision;
};

static void format_check_printf_arg_type(
		char fmt,
		type *const t_arg,
		where *loc_expr,
		type *const t_expected,
		where *loc_str)
{
	unsigned char *const default_warningp = &cc1_warning.attr_printf_bad;
	unsigned char *warningp = default_warningp;

#if 0
		case 'p':
			prim = type_void;

			if(cc1_warning.attr_printf_voidp){
				/* strict %p / void* check - emitted with voidp flag */
				warningp = &cc1_warning.attr_printf_voidp;
			}else{
				/* allow any* */
				if(type_is_ptr(t_in))
					break;

				/* not a pointer - emit with the default-warning flag */
			}
			goto ptr;
#endif

	if(!(type_cmp(t_arg, t_expected, 0) & TYPE_EQUAL_ANY)){
		char buf1[TYPE_STATIC_BUFSIZ];
		char buf2[TYPE_STATIC_BUFSIZ];

		if(cc1_warn_at_w(loc_str, warningp,
				"format %%%c expects %s argument, not %s",
				fmt,
				type_to_str_r(buf1, t_expected),
				type_to_str_r(buf2, t_arg)))
		{
			note_at(loc_expr, "argument here");
		}
	}
}

static int increment_str(size_t *i, size_t len)
{
	return ++*i < len;
}

static int parse_digit(const char *fmt, size_t *const i, size_t const len, int *const got_asterisk)
{
	if(fmt[*i] == '*'){
		*got_asterisk = 1;
		return increment_str(i, len);
	}

	while(isdigit(fmt[*i])){
		/* ignore the width itself */
		if(!increment_str(i, len))
			return 0;
	}
	return 1;
}

static const char *parse_format_arg(const char *fmt, size_t *const i, size_t const len, struct format_arg *const out)
{
	/*
	 * 0-n of: '#' '0' '-' ' ' '+' '\'' for flags
	 * 0-n of: [0-9] for field width OR \* for int arg
	 * \.[0-9]* for precision OR \.\* for int arg
	 * <length modifier> for length modifier
	 * [diouxX]
	 * [eEfFgGaA] - double
	 * [c]  - int
	 * [s]  - char*
	 * [p]  - void*
	 * [n]  - int*
	 * [m]  - nothing <strerror(errno), linux only>
	 * [%]  - nothing
	 */
	enum {
		length_none,
		length_h,
		length_hh,
		length_l,
		length_ll,
		length_L,
		length_j,
		length_t,
		length_z
	} lengthmod = length_none;

	memset(out, 0, sizeof *out);

	while(strchr("#0- +'", fmt[*i])){
		/* ignore the modifier itself */
		if(!increment_str(i, len))
			return "invalid modifier character";
	}

	if(!parse_digit(fmt, i, len, &out->expected_field_width))
		return "invalid field width";

	if(fmt[*i] == '.'){
		if(!increment_str(i, len))
			return "incomplete format specifier (missing precision)";
		if(!parse_digit(fmt, i, len, &out->expected_precision))
			return "invalid precision";
	}

	/* length modifier */
	switch(fmt[*i]){
		case 'h':
			if(*i + 1 < len && fmt[*i+1] == 'h')
				lengthmod = length_hh;
			else
				lengthmod = length_h;
			break;
		case 'l':
			if(*i + 1 < len && fmt[*i+1] == 'l')
				lengthmod = length_ll;
			else
				lengthmod = length_l;
			break;
		case 'L':
			lengthmod = length_L;
			break;
		case 'j':
			lengthmod = length_j;
			break;
		case 't':
			lengthmod = length_t;
			break;
		case 'z':
			lengthmod = length_z;
			break;
	}
	if(lengthmod != length_none && !increment_str(i, len))
		return "incomplete format specifier";

	switch(fmt[*i]){
		enum type_primitive btype;

		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
		case 'n':
			switch(lengthmod){
				case length_none: btype = type_int; break;
				case length_hh:   btype = type_schar; break;
				case length_h:    btype = type_short; break;
				case length_l:    btype = type_long; break;
				case length_ll:   btype = type_llong; break;
				case length_j:    btype = type_intmax_t; break;
				case length_z:    btype = type_size_t; break;
				case length_t:    btype = type_ptrdiff_t; break;
				default:
invalid_lengthmod:
					return "invalid length modifier for integer format";
			}
			switch(fmt[*i]){
				case 'd':
				case 'i':
					/* signed */
					break;
				case 'n':
					/* pointerify done in a moment */
					break;
				default:
					btype = TYPE_PRIMITIVE_TO_UNSIGNED(btype);
					break;
			}

			out->expected_type = type_nav_btype(cc1_type_nav, btype);
			if(fmt[*i] == 'n')
				out->expected_type = type_ptr_to(out->expected_type);
			break;

		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		case 'a':
		case 'A':
			switch(lengthmod){
				case length_none:
				case length_l:
					btype = type_double;
					break;

				case length_L:
					btype = type_ldouble;
					break;

				default:
					goto invalid_lengthmod;
			}
			out->expected_type = type_nav_btype(cc1_type_nav, btype);
			break;

		case 'c':
		case 's':
			switch(lengthmod){
				case length_none:
					btype = type_nchar;
					break;

				case length_l:
					btype = TYPE_WCHAR();
					break;

				default:
					goto invalid_lengthmod;
			}
			out->expected_type = type_nav_btype(cc1_type_nav, btype);
			if(fmt[*i] == 's')
				out->expected_type = type_ptr_to(out->expected_type);
			break;

		case 'p':
			if(lengthmod != length_none)
				goto invalid_lengthmod;
			out->expected_type = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));
			break;

		case 'm':
			if(platform_sys() != SYS_linux)
				return "%m used on non-linux system";
			/* fallthrough */
		case '%':
			if(lengthmod != length_none)
				goto invalid_lengthmod;
			out->expected_type = NULL;
			break;

		default:
			return "invalid conversion character";
	}

	return NULL;
}

static void format_check_printf_arg(
		char fmt,
		where *strloc,
		expr ***current_arg,
		type *expected_type,
		const char *desc)
{
	expr *e = **current_arg;

	if(!e){
		cc1_warn_at(strloc, attr_printf_bad,
				"too few arguments for %s (%%%c)",
				desc, fmt);
		return;
	}

	format_check_printf_arg_type(fmt, e->tree_type, &e->where, expected_type, strloc);

	++*current_arg;
}

static void format_check_printf_str(
		expr **args,
		const char *fmt, const size_t len,
		const int var_idx,
		where *quote_loc)
{
	expr **current_arg = args;
	size_t i;

	for(i = var_idx; *current_arg && i > 0; i--)
		current_arg++;

	for(i = 0; i < len && fmt[i];){
		const char *err;
		where strloc;
		struct format_arg parsed;

		if(fmt[i++] != '%')
			continue;

		if(i == len)
			err = "incomplete format specifier";
		else
			err = parse_format_arg(fmt, &i, len, &parsed);

		strloc = *quote_loc;
		strloc.chr += i + 1;

		if(DEBUG_FORMAT_CHECK && !err){
			fprintf(stderr, "%s: format check: field-width=%d, precision=%d type=%s\n",
					where_str(&strloc),
					parsed.expected_field_width,
					parsed.expected_precision,
					type_to_str(parsed.expected_type));
		}

		if(err){
			cc1_warn_at(&strloc, attr_printf_bad, "%s", err);
			return;
		}

		if(var_idx == -1){
			/* we don't have any arguments to check, done all we can do */
			continue;
		}

		if(parsed.expected_field_width){
			format_check_printf_arg(
					fmt[i],
					&strloc,
					&current_arg,
					type_nav_btype(cc1_type_nav, type_int),
					"field width");
		}
		if(parsed.expected_precision){
			format_check_printf_arg(
					fmt[i],
					&strloc,
					&current_arg,
					type_nav_btype(cc1_type_nav, type_int),
					"precision");
		}

		format_check_printf_arg(
				fmt[i],
				&strloc,
				&current_arg,
				parsed.expected_type,
				"format");
	}

	if(i > len)
		i = len;

	if(var_idx != -1 && (!fmt[i] || i == len) && *current_arg){
		cc1_warn_at(&(*current_arg)->where, attr_printf_toomany,
				"too many arguments for format");
	}
}

static void format_check_printf(
		expr *str_arg,
		expr **args,
		unsigned var_idx)
{
	stringlit *fmt_str;
	consty k;

	const_fold(str_arg, &k);

	switch(k.type){
		case CONST_NO:
		case CONST_NEED_ADDR:
			/* check for the common case printf(x?"":"", ...) */
			if(expr_kind(str_arg, if)){

				format_check_printf(
						str_arg->lhs ? str_arg->lhs : str_arg->expr,
						args, var_idx);

				format_check_printf(str_arg->rhs, args, var_idx);

				return;
			}
			goto not_string;

		case CONST_NUM:
			if(K_INTEGRAL(k.bits.num) && k.bits.num.val.i == 0)
				return; /* printf(NULL, ...) */
			/* fall - printf(5, ...) or printf(2.3f, ...) */

		case CONST_ADDR:
not_string:
			cc1_warn_at(&str_arg->where, attr_printf_bad,
					"format argument isn't a string constant");
			return;

		case CONST_STRK:
			fmt_str = k.bits.str->lit;
			break;
	}

	if(fmt_str->cstr->type != CSTRING_WIDE){
		const char *fmt = fmt_str->cstr->bits.ascii;
		const size_t len = fmt_str->cstr->count - 1;

		if(len == 0)
			;
		else if(k.offset < 0 || (size_t)k.offset >= len)
			cc1_warn_at(&str_arg->where, attr_printf_bad,
					"undefined printf-format argument");
		else
			format_check_printf_str(args, fmt + k.offset, len,
					var_idx, &str_arg->where);
	}
}

void format_check_call(
		type *fnty, expr **args, const int variadic)
{
	attribute *attr = type_attr_present(fnty, attr_format);
	int n, fmt_idx, var_idx;

	if(!attr || !variadic)
		return;
	switch(attr->bits.format.validity){
		case fmt_unchecked:
			/*ICW("unchecked __attribute__((format...))");*/
			/* printf checking is disabled/warning is off - return */
			/* fall */
		case fmt_invalid:
			return;
		case fmt_valid:
			break;
	}

	fmt_idx = attr->bits.format.fmt_idx;
	var_idx = attr->bits.format.var_idx;

	n = dynarray_count(args);

	/* do bounds checks here, but no warnings
	 * warnings are on the decl
	 *
	 * if var_idx is zero we only check the format string,
	 * not the arguments
	 */
	if(fmt_idx >= n
	|| var_idx > n
	|| (var_idx > -1 && var_idx <= fmt_idx))
	{
		return;
	}

	switch(attr->bits.format.fmt_func){
		case attr_fmt_printf:
			if(cc1_warning.attr_printf_bad
			|| cc1_warning.attr_printf_toomany
			|| cc1_warning.attr_printf_unknown)
			{
				format_check_printf(args[fmt_idx], args, var_idx);
			}
			break;

		case attr_fmt_scanf:
			ICW("scanf check");
			break;
	}
}

void format_check_decl(decl *d, attribute *da)
{
	type *r_func;
	funcargs *fargs;
	int fmt_idx, var_idx, nargs;

	if(da->bits.format.validity != fmt_unchecked){
		/* i.e. checked */
		return;
	}

	r_func = type_is_func_or_block(d->ref);
	assert(r_func);
	fargs = r_func->bits.func.args;

	if(!fargs->variadic){
		/* if the index is zero, we ignore it, e.g.
		 * vprintf(char *, va_list) __attribute((format(printf, 1, 0)));
		 *                                                         ^
		 *
		 * (-1, not zero, since we subtract one for format indexes)
		 */
		if(da->bits.format.var_idx >= 0){
			cc1_warn_at(&da->where, attr_printf_bad,
					"variadic function required for format attribute");
		}
		goto invalid;
	}

	nargs = dynarray_count(fargs->arglist);
	fmt_idx = da->bits.format.fmt_idx;
	var_idx = da->bits.format.var_idx;

	/* format string index must be < nargs */
	if(fmt_idx >= nargs){
		cc1_warn_at(&da->where,
				attr_format_baddecl,
				"format argument out of bounds (%d >= %d)",
				fmt_idx, nargs);
		goto invalid;
	}

	if(var_idx != -1){
		/* variadic index must be nargs */
		if(var_idx != nargs){
			cc1_warn_at(&da->where,
					attr_printf_bad,
					"variadic argument out of bounds (should be %d)",
					nargs + 1); /* +1 to get to the "..." index as 1-based */
			goto invalid;
		}

		assert(var_idx > fmt_idx);
	}

	if(type_str_type(fargs->arglist[fmt_idx]->ref) != type_str_char){
		cc1_warn_at(&da->where, attr_printf_bad,
				"format argument not a string type");
		goto invalid;
	}

	da->bits.format.validity = fmt_valid;
	return;
invalid:
	da->bits.format.validity = fmt_invalid;
}
