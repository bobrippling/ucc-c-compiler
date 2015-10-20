#include <stdio.h>
#include <string.h>

#include "../util/std.h"
#include "../cc1/attributes.h"
#include "../cc1/ops/__builtins.h"

#include "has.h"

#include "feat.h"
#include "main.h"
#include "include.h"

typedef int has_fn(const char *nam);

static has_fn has_feature;
static has_fn has_extension;
static has_fn has_attribute;
static has_fn has_builtin;

static const struct has_tbl
{
	const char *nam;
	has_fn *handler;
} has_tbl[] = {
#define HAS(x) { #x, has_ ## x }
	HAS(feature),
	HAS(extension),
	HAS(attribute),
	HAS(builtin),
#undef HAS
	{ NULL, NULL }
};

int has_func(const char *fn, const char *arg1)
{
	const struct has_tbl *p;

	if(!strncmp(fn, "__has_", 6))
		fn += 6;

	for(p = has_tbl; p->nam; p++)
		if(!strcmp(p->nam, fn))
			return p->handler(arg1);

	return 0;
}

static int has_feat_ext(const char *nam, int as_ext)
{
	static const struct c_tbl
	{
		enum c_std std;
		const char *nam;
		int has;
		int ignore_ext;
	} tbl[] = {
		{ STD_C11, "c_alignas", 1, 0 },
		{ STD_C11, "c_alignof", 1, 0 },
		{ STD_C11, "c_generic_selections", 1, 0 },
		{ STD_C11, "c_static_assert", 1, 0 },
		{ STD_C11, "c_atomic", UCC_HAS_ATOMICS, 0 },
		{ STD_C11, "c_thread_local", UCC_HAS_THREADS, 0 },
		{ STD_C11, "c_complex", UCC_HAS_COMPLEX, 0 },

		{ STD_C99, "c_vla", UCC_HAS_VLA, 0 },

		/* compat with clang */
		{ STD_C89, "address_sanitizer", 0, 1 },
		{ STD_C89, "enumerator_attributes", 1, 1 },

		{ STD_C89, "blocks", 1, 1 },

		{ STD_C89, NULL, 0, 0 }
	};
	const struct c_tbl *p;

	for(p = tbl; p->nam; p++){
		if(!strcmp(p->nam, nam)){
			if(!p->ignore_ext && !as_ext)
				continue;
			if(cpp_std < p->std)
				continue;

			return p->has;
		}
	}

	return 0;
}

static int has_extension(const char *nam)
{
	return has_feat_ext(nam, 1);
}

static int has_feature(const char *nam)
{
	return has_feat_ext(nam, 0);
}

static int has_attribute(const char *nam)
{
#define NAME(x) if(!strcmp(nam, #x) || !strcmp("__" #x "__", nam)) return 1;
#define ALIAS(s, x) if(!strcmp(nam, s) || !strcmp("__" s "__", nam)) return 1;
#define EXTRA_ALIAS ALIAS
	ATTRIBUTES
#undef NAME
#undef ALIAS
#undef EXTRA_ALIAS
	return 0;
}

static int has_builtin(const char *nam)
{
#define BUILTIN(x, ty) if(!strcmp("__builtin_" x, nam)) return 1;
	BUILTINS
#undef BUILTIN
	return 0;
}
