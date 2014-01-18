#ifndef TYPE_IS_H
#define TYPE_IS_H

struct decl;

type *type_skip_tdefs_casts(type *r);

struct decl *type_is_tdef(type *r);

type *type_next(type *r);
type *type_is(type *r, enum type_kind t);
type *type_is_primitive(type *r, enum type_primitive p);
type *type_is_ptr(type *r);
type *type_is_array(type *r);
type *type_is_scalar(type *r);

const btype *type_get_type(type *r);

int type_is_bool(type *r);
int type_is_fptr(type *r);
int type_is_nonfptr(type *r);
int type_is_void_ptr(type *r);
int type_is_nonvoid_ptr(type *r);
int type_is_integral(type *r);

/* e.g. char, short, float -> int, int, double */
int type_is_promotable(type *, type **pto);

unsigned type_align(type *r, where *from);

int type_is_complete(type *r);
int type_is_incomplete_array(type *r);

type *type_complete_array(type *r, int sz);

struct struct_union_enum_st *type_is_s_or_u_or_e(type *r);
struct struct_union_enum_st *type_is_s_or_u(type *r);

type *type_func_call(type *fp, struct funcargs **pfuncargs);

int type_decayable(type *r);
type *type_decay(type *const r);

int type_is_void(type *r);
int type_is_signed(type *r);
int type_is_floating(type *r);

enum type_qualifier type_qual(const type *r);
enum type_primitive type_primitive(type *ty);

struct funcargs *type_funcargs(type *r);

int type_is_callable(type *r);
int type_is_const(type *r);

unsigned type_array_len(type *r);

int type_is_variadic_func(type *);

/* type_is_* */
int type_is_complete(type *);
int type_is_variably_modified(type *);
int type_is_void(    type *);
int type_is_integral(type *);
int type_is_bool(    type *);
int type_is_signed(  type *);
int type_is_floating(type *);
int type_is_const(   type *);
int type_is_callable(type *);
int type_is_fptr(    type *);
int type_is_void_ptr(type *);
int type_is_nonvoid_ptr(type *);

int type_is_variadic_func(type *);

/* type_is_* */
int type_is_complete(type *);
int type_is_variably_modified(type *);
int type_is_void(    type *);
int type_is_integral(type *);
int type_is_bool(    type *);
int type_is_signed(  type *);
int type_is_floating(type *);
int type_is_const(   type *);
int type_is_callable(type *);
int type_is_fptr(    type *);
int type_is_void_ptr(type *);
int type_is_nonvoid_ptr(type *);
int type_is_incomplete_array(type *);

type *type_is_decayed_array(type *);

type *type_is(type *, enum type_kind);
type *type_is_primitive(type *, enum type_primitive);
struct decl     *type_is_tdef(type *);
type *type_is_ptr(type *); /* returns r->ref iff ptr */
type *type_is_ptr_or_block(type *);
int       type_is_nonfptr(type *);
type *type_is_array(type *); /* returns r->ref iff array */

type *type_is_scalar(type *);
type *type_is_func_or_block(type *);
struct struct_union_enum_st *type_is_s_or_u(type *);
struct struct_union_enum_st *type_is_s_or_u_or_e(type *);

#endif
