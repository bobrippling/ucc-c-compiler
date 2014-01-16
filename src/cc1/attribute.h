#ifndef ATTR_H
#define ATTR_H

#include "type.h"

typedef struct attribute attribute;
struct attribute
{
	where where;

	enum attribute_type
	{
		attr_format,
		attr_unused,
		attr_warn_unused,
		attr_section,
		attr_enum_bitmask,
		attr_noreturn,
		attr_noderef,
		attr_call_conv,
		attr_nonnull,
		attr_packed,
		attr_sentinel,
		attr_aligned,
		attr_LAST
		/*
		 * TODO: warning
		 * pure - no globals
		 * const - pure + no pointers
		 */
	} type;

	union
	{
		struct
		{
			enum fmt_type
			{
				attr_fmt_printf, attr_fmt_scanf
			} fmt_func;
			int fmt_idx, var_idx, valid;
		} format;
		char *section;
		enum calling_conv
		{
			conv_x64_sysv, /* Linux, FreeBSD and Mac OS X, x64 */
			conv_x64_ms,   /* Windows x64 */
			conv_cdecl,    /* All 32-bit x86 systems, stack, caller cleanup */
			conv_stdcall,  /* Windows x86 stack, callee cleanup */
			conv_fastcall  /* Windows x86, ecx, edx, caller cleanup */
		} conv;
		unsigned long nonnull_args; /* limits to sizeof(long)*8 args, i.e. 64 */
		struct expr *align, *sentinel;
	} bits;

	attribute *next;
};

attribute   *attribute_new(enum attribute_type);
void         attribute_append(attribute **loc, attribute *new);
const char  *attribute_to_str(attribute *da);

attribute *attr_present(attribute *, enum attribute_type);
attribute *type_attr_present(struct type *, enum attribute_type);
attribute *attribute_present(struct decl *, enum attribute_type);
attribute *expr_attr_present(struct expr *, enum attribute_type);

void attribute_free(struct attribute *a);

#endif
