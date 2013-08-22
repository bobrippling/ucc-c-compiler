#ifndef TYPE_REF_IS_H
#define TYPE_REF_IS_H

type_ref *type_ref_skip_tdefs_casts(type_ref *r);

type_ref *type_ref_skip_casts(type_ref *r);
decl *type_ref_is_tdef(type_ref *r);

type_ref *type_ref_next(type_ref *r);
type_ref *type_ref_is(type_ref *r, enum type_ref_type t);
type_ref *type_ref_is_type(type_ref *r, enum type_primitive p);
type_ref *type_ref_is_ptr(type_ref *r);
type_ref *type_ref_is_array(type_ref *r);
type_ref *type_ref_is_scalar(type_ref *r);

const type *type_ref_get_type(type_ref *r);

int type_ref_is_bool(type_ref *r);
int type_ref_is_fptr(type_ref *r);
int type_ref_is_nonfptr(type_ref *r);
int type_ref_is_void_ptr(type_ref *r);
int type_ref_is_nonvoid_ptr(type_ref *r);
int type_ref_is_integral(type_ref *r);

/* e.g. char, short, float -> int, int, double */
int type_ref_is_promotable(type_ref *, type_ref **pto);

unsigned type_ref_align(type_ref *r, where *from);

int type_ref_is_complete(type_ref *r);
int type_ref_is_incomplete_array(type_ref *r);

type_ref *type_ref_complete_array(type_ref *r, int sz);

struct_union_enum_st *type_ref_is_s_or_u_or_e(type_ref *r);
struct_union_enum_st *type_ref_is_s_or_u(type_ref *r);

type_ref *type_ref_func_call(type_ref *fp, funcargs **pfuncargs);

int type_ref_decayable(type_ref *r);
type_ref *type_ref_decay(type_ref *const r);

int type_ref_is_void(type_ref *r);
int type_ref_is_signed(type_ref *r);
int type_ref_is_floating(type_ref *r);

enum type_qualifier type_ref_qual(const type_ref *r);
enum type_primitive type_ref_primitive(type_ref *ty);

funcargs *type_ref_funcargs(type_ref *r);

int type_ref_is_callable(type_ref *r);
int type_ref_is_const(type_ref *r);

unsigned type_ref_array_len(type_ref *r);

#endif
