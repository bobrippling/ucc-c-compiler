#ifndef TYPE_IS_H
#define TYPE_IS_H

struct decl;

type *type_skip_all(type *r);
type *type_skip_non_tdefs(type *);
type *type_skip_non_attr(type *);
type *type_skip_non_casts(type *);
type *type_skip_wheres(type *);
type *type_skip_non_wheres(type *);

const btype *type_get_type(type *r);

type *type_complete_array(type *r, struct expr *sz);

type *type_func_call(type *fp, struct funcargs **pfuncargs);

int type_decayable(type *r);
type *type_decay(type *const r);

enum type_qualifier type_qual(const type *r);
enum type_primitive type_primitive(type *ty);

struct funcargs *type_funcargs(type *r);

unsigned type_array_len(type *r);
type *type_next(type *r);

int type_is_arith(type *);
int type_is_autotype(type *);
int type_is_bool(type *);
int type_is_callable(type *);
int type_is_complete(type *);
int type_is_const(type *);
int type_is_floating(type *);
int type_is_fptr(type *);
int type_is_incomplete_array(type *);
int type_is_integral(type *);
int type_is_nonfptr(type *);
int type_is_nonvoid_ptr(type *);
int type_is_promotable(type *, type **pto); /* e.g. char, short, float -> int, int, double */
int type_is_signed(type *);
int type_is_variadic_func(type *);
int type_is_void(type *);
int type_is_void_ptr(type *);
int type_is_variably_modified(type *);
int type_is_variably_modified_vla(type *, int *vla);

struct decl *type_is_tdef(type *);
struct struct_union_enum_st *type_is_s_or_u(type *);
struct struct_union_enum_st *type_is_s_or_u_or_e(type *);
struct struct_union_enum_st *type_is_enum(type *);

type *type_is(type *, enum type_kind);
type *type_is_array(type *); /* returns r->ref iff array */
type *type_is_decayed_array(type *);
type *type_is_func_or_block(type *);
type *type_is_primitive(type *, enum type_primitive);
type *type_is_primitive_anysign(type *ty, enum type_primitive);
type *type_is_ptr(type *); /* returns r->ref iff ptr */
type *type_is_ptr_or_block(type *);
type *type_is_scalar(type *);

enum vla_kind
{
	VLA_ANY_DIMENSION,
	VLA_TOP_DIMENSION
};
type *type_is_vla(type *, enum vla_kind);

#endif
