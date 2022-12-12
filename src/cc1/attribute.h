#ifndef ATTR_H
#define ATTR_H

#include "type.h"
#include "retain.h"
#include "attributes.h"
#include "visibility.h"
#include "enum_no_sanitize.h"

typedef struct attribute attribute;
struct attribute
{
	where where;

	struct retain rc;

	enum attribute_type
	{
#define NAME(x, typrop, tymismatch) attr_ ## x,
#define RENAME(s, x, typrop, tymismatch) attr_ ## x,
#define ALIAS(s, x)
#define COMPLEX_ALIAS(s, x)
		ATTRIBUTES
#undef NAME
#undef RENAME
#undef ALIAS
#undef COMPLEX_ALIAS
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
			enum
			{
				fmt_unchecked, fmt_valid, fmt_invalid
			} validity;
			struct expr  *fmt_idx, *var_idx;
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
		struct expr *align, *sentinel, *priority;
		struct decl *cleanup;
		int ucc_debugged;
		enum visibility visibility;
		struct decl *alias;
		enum no_sanitize no_sanitize;
	} bits;
};

attribute   *attribute_new(enum attribute_type);
const char  *attribute_to_str(attribute *da);

attribute *attr_present(attribute **, enum attribute_type);
attribute *type_attr_present(struct type *, enum attribute_type);
attribute *attribute_present(struct decl *, enum attribute_type);
attribute *expr_attr_present(struct expr *, enum attribute_type);

int attribute_equal(attribute *, attribute *);

int attribute_is_typrop(attribute *);

void attribute_free(struct attribute *a);
struct attribute **attribute_array_retain(struct attribute **);
void attribute_array_release(struct attribute ***);
void attribute_debug_check(struct attribute **attr);

#endif
