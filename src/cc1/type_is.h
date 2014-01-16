#ifndef TYPE_REF_IS_H
#define TYPE_REF_IS_H

type *type_skip_tdefs_casts(type *r);

type *type_skip_casts(type *r);
decl *type_is_tdef(type *r);

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

struct_union_enum_st *type_is_s_or_u_or_e(type *r);
struct_union_enum_st *type_is_s_or_u(type *r);

type *type_func_call(type *fp, funcargs **pfuncargs);

int type_decayable(type *r);
type *type_decay(type *const r);

int type_is_void(type *r);
int type_is_signed(type *r);
int type_is_floating(type *r);

enum type_qualifier type_qual(const type *r);
enum type_primitive type_primitive(type *ty);

funcargs *type_funcargs(type *r);

int type_is_callable(type *r);
int type_is_const(type *r);

unsigned type_array_len(type *r);

#endif
