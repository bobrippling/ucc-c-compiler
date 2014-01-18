#ifndef TYPE_ROOT_H
#define TYPE_ROOT_H

#include "btype.h"
#include "sue.h"
#include "type.h"
#include "decl.h"
#include "expr.h"

struct type_nav;

extern struct type_nav *cc1_type_nav;

struct type_nav *type_nav_init(void);

type *type_nav_btype(struct type_nav *root, enum type_primitive p);
type *type_nav_suetype(struct type_nav *root, struct_union_enum_st *);

type *type_nav_va_list(struct type_nav *root);
type *type_decay(type *);

type *type_ptr_to(type *);
type *type_pointed_to(type *);
type *type_block_of(type *);

type *type_array_of(type *, struct expr *sz);
type *type_array_of_qual(
		type *, struct expr *sz,
		enum type_qualifier, int is_static);

type *type_func_of(type *, struct funcargs *args);
type *type_called(type *, struct funcargs **pfuncargs);

type *type_tdef_of(expr *, decl *);

type *type_unqualify(type *);
type *type_qualify(type *, enum type_qualifier);
type *type_sign(type *, int is_signed);

type *type_attributed(type *, attribute *);

type *type_nav_MAX_FOR(struct type_nav *, unsigned sz);

#if 0
type *type_new_tdef(expr *, decl *);
type *type_new_type(const btype *);
type *type_new_type_primitive(enum type_primitive);
type *type_new_type_qual(enum type_primitive, enum type_qualifier);
type *type_new_ptr(  type *to, enum type_qualifier);
type *type_new_block(type *to, enum type_qualifier);
type *type_new_array(type *to, expr *sz);
type *type_new_array2(type *to, expr *sz, enum type_qualifier, int is_static);
type *type_new_func(type *, funcargs *, symtable *arg_scope);
type *type_new_cast( type *from, enum type_qualifier new);
type *type_new_cast_signed(type *from, int is_signed);
type *type_new_cast_add(type *from, enum type_qualifier extra);
#endif

#if 0
type *type_complete_array(type *r, int sz) ucc_wur;
enum type_qualifier type_qual(const type *);

funcargs *type_funcargs(type *);
enum type_primitive type_primitive(type *);

unsigned type_align(type *, where *from);
unsigned type_array_len(type *);

int       type_decayable(type *r);
type *type_decay(type *);
#endif

#if 0
const char *type_to_str_r_spel(char buf[ucc_static_param TYPE_STATIC_BUFSIZ], type *r, char *spel);
const char *type_to_str_r(char buf[ucc_static_param TYPE_STATIC_BUFSIZ], type *r);
const char *type_to_str_r_show_decayed(char buf[ucc_static_param TYPE_STATIC_BUFSIZ], type *r);
const char *type_to_str(type *);
#endif

#endif
