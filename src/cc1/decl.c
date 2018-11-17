#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/platform.h"
#include "../util/dynarray.h"

#include "cc1_where.h"
#include "fopt.h"

#include "macros.h"
#include "sue.h"
#include "const.h"
#include "cc1.h"
#include "fold.h"
#include "funcargs.h"
#include "defs.h"
#include "mangle.h"

#include "type_is.h"
#include "type_nav.h"

decl *decl_new_w(const where *w)
{
	decl *d = umalloc(sizeof *d);
	memcpy_safe(&d->where, w);
	return d;
}

decl *decl_new()
{
	where wtmp;
	where_cc1_current(&wtmp);
	return decl_new_w(&wtmp);
}

decl *decl_new_ty_sp(type *ty, char *sp)
{
	decl *d = decl_new();
	d->ref = ty;
	d->spel = sp;
	return d;
}

void decl_replace_with(decl *to, decl *from)
{
	attribute **i;

	/* XXX: memleak of .ref */
	memcpy_safe(&to->where, &from->where);
	to->ref      = from->ref;
	to->spel_asm = from->spel_asm, from->spel_asm = NULL;
	/* no point copying bitfield stuff */
	memcpy_safe(&to->bits, &from->bits);

	for(i = from->attr; i && *i; i++)
		dynarray_add(&to->attr, RETAIN(*i));
}

const char *decl_asm_spel(decl *d)
{
	if(!d->spel_asm)
		d->spel_asm = func_mangle(d->spel, type_is(d->ref, type_func));

	return d->spel_asm;
}

void decl_free(decl *d)
{
	if(!d)
		return;

	/* expr_free(d->field_width); XXX: leak */

	free(d);
}

const char *decl_store_to_str(const enum decl_storage s)
{
	static char buf[16]; /* "inline register" is the longest - just a fit */

	if(s & STORE_MASK_EXTRA){
		char *trail_space = NULL;
		*buf = '\0';

		if((s & STORE_MASK_EXTRA) == store_inline){
			strcpy(buf, "inline ");
			trail_space = buf + strlen("inline");
		}

		strcpy(buf + strlen(buf), decl_store_to_str(s & STORE_MASK_STORE));

		if(trail_space && trail_space[1] == '\0')
			*trail_space = '\0';

		return buf;
	}

	switch(s){
		case store_inline:
			ICE("inline");
		case store_default:
			return "";
		CASE_STR_PREFIX(store, auto);
		CASE_STR_PREFIX(store, static);
		CASE_STR_PREFIX(store, extern);
		CASE_STR_PREFIX(store, register);
		CASE_STR_PREFIX(store, typedef);
	}
	return NULL;
}

unsigned decl_size(decl *d)
{
	if(type_is_void(d->ref))
		die_at(&d->where, "%s is void", d->spel);

	if(!type_is(d->ref, type_func) && d->bits.var.field_width)
		die_at(&d->where, "can't take size of a bitfield");

	return type_size(d->ref, &d->where);
}

type *decl_type_for_bitfield(decl *d)
{
	assert(!type_is(d->ref, type_func));

	if(d->bits.var.field_width){
		const unsigned bits = const_fold_val_i(d->bits.var.field_width);
		const int is_signed = type_is_signed(d->ref);
		unsigned bytes = bits / CHAR_BIT;

		/* need to add on a byte for any leftovers */
		if(bits % CHAR_BIT)
			bytes++;

		return type_nav_MAX_FOR(cc1_type_nav, bytes, is_signed);
	}else{
		return d->ref;
	}
}

void decl_size_align_inc_bitfield(decl *d, unsigned *const sz, unsigned *const align)
{
	type *ty = decl_type_for_bitfield(d);

	*sz = type_size(ty, NULL);
	*align = type_align(ty, NULL);
}

static unsigned decl_align1(decl *d)
{
	unsigned al = 0;

	if(!type_is(d->ref, type_func) && d->bits.var.align.resolved)
		al = d->bits.var.align.resolved;

	return al ? al : type_align(d->ref, &d->where);
}

unsigned decl_align(decl *d)
{
	unsigned max = 0;
	decl *i = decl_proto(d);

	for(; i; i = i->impl){
		unsigned cur = decl_align1(i);
		if(cur > max){
			max = cur;
		}
	}

	return max;
}

enum type_cmp decl_cmp(decl *a, decl *b, enum type_cmp_opts opts)
{
	enum type_cmp cmp = type_cmp(a->ref, b->ref, opts);
	enum decl_storage sa = a->store & STORE_MASK_STORE,
	                  sb = b->store & STORE_MASK_STORE;

	if(cmp & TYPE_EQUAL_ANY && sa != sb){
		/* types are equal but there's a store mismatch
		 * only return convertible if it's a typedef or static mismatch
		 */
#define STORE_INCOMPAT(st) ((st) == store_typedef || (st) == store_static)

		if(STORE_INCOMPAT(sa) || STORE_INCOMPAT(sb))
			return TYPE_CONVERTIBLE_IMPLICIT;
	}

	return cmp;
}

unsigned decl_hash(const decl *d)
{
	unsigned hash = type_hash(d->ref);

	hash ^= d->store;

	if(d->spel)
		hash ^= dynmap_strhash(d->spel);

	return hash;
}

int decl_conv_array_func_to_ptr(decl *d)
{
	type *old = d->ref;

	d->ref = type_decay(d->ref);

	return old != d->ref;
}

type *decl_is_decayed_array(decl *d)
{
	return type_is_decayed_array(d->ref);
}

int decl_store_static_or_extern(enum decl_storage s)
{
	switch((enum decl_storage)(s & STORE_MASK_STORE)){
		case store_static:
		case store_extern:
			return 1;
		default:
			return 0;
	}
}

enum linkage decl_linkage(decl *d)
{
	/* global scoped or extern */
	decl *p = decl_proto(d);

	/* if first instance is static, we're internal */
	switch((enum decl_storage)(p->store & STORE_MASK_STORE)){
		case store_extern: return linkage_external;
		case store_static: return linkage_internal;

		case store_register:
		case store_auto:
		case store_typedef:
			return linkage_none;

		case store_inline:
			ICE("bad store");

		case store_default:
			break;
	}

	/* either global non-static or local */
	return d->sym && d->sym->type == sym_global
		? linkage_external
		: linkage_none;
}

int decl_store_duration_is_static(decl *d)
{
	return decl_store_static_or_extern(d->store)
		|| (d->sym && d->sym->type == sym_global);
}

const char *decl_store_spel_type_to_str_r(
		char buf[DECL_STATIC_BUFSIZ],
		enum decl_storage store,
		const char *spel,
		type *ty)
{
	char *bufp = buf;

	if(store)
		bufp += snprintf(bufp, DECL_STATIC_BUFSIZ, "%s ", decl_store_to_str(store));

	type_to_str_r_spel(bufp, ty, spel);

	return buf;
}

const char *decl_to_str_r(char buf[DECL_STATIC_BUFSIZ], decl *d)
{
	return decl_store_spel_type_to_str_r(buf, d->store, d->spel, d->ref);
}

const char *decl_to_str(decl *d)
{
	static char buf[DECL_STATIC_BUFSIZ];
	return decl_to_str_r(buf, d);
}

decl *decl_proto(decl *const d)
{
	decl *i;

	for(i = d; i->proto; i = i->proto);

	return i;
}

decl *decl_impl(decl *const d)
{
	decl *i;

	for(i = d; i; i = i->proto)
		if(i->bits.func.code)
			return i;

	for(i = d; i; i = i->impl)
		if(i->bits.func.code)
			return i;

	return d;
}

int decl_is_pure_inline(decl *const d)
{
	/*
	 * inline semantics
	 *
	 * "" = inline only
	 * "static" = code emitted, decl is static
	 * "extern" mentioned, or "inline" not mentioned = code emitted, decl is extern
	 *
	 * look for non-inline store on any prototypes
	 */
	decl *i;

#define CHECK_INLINE(i)                                      \
		if(!(i->store & store_inline))                           \
			return 0; /* not marked as inline - not pure inline */ \
                                                             \
		if((i->store & STORE_MASK_STORE) != store_default)       \
			return 0; /* static or extern */

	for(i = d; i; i = i->proto){
		CHECK_INLINE(i);
	}

	for(i = d; i; i = i->impl){
		CHECK_INLINE(i);
	}

	return 1;
}

int decl_should_emit_code(decl *d)
{
	assert(type_is(d->ref, type_func));
	return d->bits.func.code && !decl_unused_and_internal(d) && !decl_is_pure_inline(d);
}

int decl_unused_and_internal(decl *d)
{
	/* need to check every clone of the decl */
	decl *i;
	int used = 0;

	for(i = d; i; i = i->proto){
		if(i->used){
			used = 1;
			goto fin;
		}
	}
	for(i = d; i; i = i->impl){
		if(i->used){
			used = 1;
			goto fin;
		}
	}

fin:
	return !used && decl_linkage(d) != linkage_external;
}

int decl_is_bitfield(decl *d)
{
	if(type_is(d->ref, type_func))
		return 0;

	return !!d->bits.var.field_width;
}

enum visibility decl_visibility(decl *d)
{
	attribute *visibility = attribute_present(d, attr_visibility);
	if(visibility)
		return visibility->bits.visibility;

	return cc1_visibility_default;
}

int decl_interposable(decl *d)
{
	/*
	 * Match gcc's -fsemantic-interposition default, where:
	 *
	 * -fpic -fpie -fsemantic-interposition function-visibility  |  can-inline-non-static function?
	 *   0     0              0                 default                     yes (-fno-pic)
	 *   0     0              1                 default                     yes (-fno-pic)
	 *   0     1              0                 default                     yes (-fno-pic)
	 *   0     1              1                 default                     yes (-fno-pic)
	 *   0     0              0             protected/hidden                yes (-fno-pic)
	 *   0     0              1             protected/hidden                yes (-fno-pic)
	 *   0     1              0             protected/hidden                yes (-fno-pic)
	 *   0     1              1             protected/hidden                yes (-fno-pic)
	 *
	 *   1     1              1             protected/hidden                yes (-fvisibility=protected/hidden)
	 *   1     1              0             protected/hidden                yes (-fvisibility=protected/hidden)
	 *   1     0              1             protected/hidden                yes (-fvisibility=protected/hidden)
	 *   1     0              0             protected/hidden                yes (-fvisibility=protected/hidden)
	 *
	 *   1     1              1                 default                     yes (-fpie)
	 *   1     1              0                 default                     yes (-fpie)
	 *   1     0              1                 default                     no
	 *   1     0              0                 default                     yes (-fno-semantic-interposition)
	 *
	 * -fno-pic: -fsemantic-interposition and -fvisibility=... have no effect
	 *
	 * -fpic:    We can inline non-static, default-visibility functions when
	 *           -fno-semantic-interposition is set otherwise, the ELF abi says a
	 *           non-static default-visibility function may be overridden.
	 */

	if(!cc1_fopt.pic || cc1_fopt.pie){
		/* !pic - not compiling for interposable shared library */
		/* pie, this is the main program, can't have its symbols interposed */
		return 0;
	}

	switch(decl_linkage(d)){
		case linkage_internal:
		case linkage_none:
			return 0; /* static decl, fixed */
		case linkage_external:
			break;
	}

	switch(decl_visibility(d)){
		case VISIBILITY_DEFAULT:
			break;
		case VISIBILITY_PROTECTED:
			return 0; /* symbol visible, but not interposable by contract */
		case VISIBILITY_HIDDEN:
			return 0; /* symbol not visible, so not interposable */
	}

	return cc1_fopt.semantic_interposition;
}
