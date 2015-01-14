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
type *type_nav_auto(struct type_nav *root); /* returns placeholder */
type *type_nav_suetype(struct type_nav *root, struct_union_enum_st *);
type *type_nav_int_enum(struct type_nav *root, struct_union_enum_st *);

type *type_nav_changeauto(type *ontop, type *trailing);

type *type_nav_va_list(struct type_nav *root, symtable *symtab);
type *type_nav_voidptr(struct type_nav *root);
type *type_decay(type *);

type *type_ptr_to(type *);
type *type_decayed_ptr_to(type *, type *array_from);
type *type_dereference_decay(type *); /* int * -> int, int (*)() -> int (*)() */
type *type_block_of(type *);

type *type_array_of(type *, struct expr *sz);
type *type_array_of_static(type *, struct expr *sz, int is_static);
type *type_vla_of(type *, struct expr *sz, int vlatype);

type *type_func_of(type *, struct funcargs *args, struct symtable *arg_scope);
type *type_called(type *, struct funcargs **pfuncargs);

type *type_tdef_of(expr *, decl *);

type *type_unqualify(type *);
type *type_qualify(type *, enum type_qualifier);
type *type_sign(struct type_nav *root, type *ty, int make_signed);

type *type_attributed(type *, attribute *);

type *type_nav_MAX_FOR(struct type_nav *, unsigned sz);

type *type_at_where(type *, where *);

void type_nav_dump(struct type_nav *);

#endif
