#include <stdlib.h>

#include "../util/dynarray.h"
#include "../util/alloc.h"

#include "type.h"
#include "sue.h"
#include "fold.h"

#include "type_nav.h"

#include "c_types.h"

type *c_types_make_va_list(symtable *symtab)
{
	/* pointer to struct __builtin_va_list */
	/* must match platform abi - vfprintf(..., ap); */
	sue_member **sue_members = NULL;

	type *void_ptr = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));

	expr *e_one = expr_compiler_generated(expr_new_val(1));

	FOLD_EXPR(e_one, symtab);

	/*
	unsigned int gp_offset;
	unsigned int fp_offset;
	void *overflow_arg_area;
	void *reg_save_area;
	*/

#define ADD_DECL(to, dcl)        \
	dynarray_add(&to,              \
	    sue_member_from_decl(dcl))

#define ADD_SCALAR(to, ty, sp)     \
	ADD_DECL(to,                     \
	    decl_new_ty_sp(              \
	      type_nav_btype(            \
	        cc1_type_nav, ty),       \
	      ustrdup(sp)))


	ADD_SCALAR(sue_members, type_uint, "gp_offset");
	ADD_SCALAR(sue_members, type_uint, "fp_offset");
	ADD_DECL(sue_members, decl_new_ty_sp(void_ptr, "overflow_arg_area"));
	ADD_DECL(sue_members, decl_new_ty_sp(void_ptr, "reg_save_area"));

	/* typedef struct __va_list_struct __builtin_va_list[1]; */
	{
		where locn = { 0 };

		struct_union_enum_st *sue = sue_predeclare(
				symtab, ustrdup("__va_list_struct"), type_struct, &locn);

		type *va_list_struct = type_nav_suetype(
				cc1_type_nav,
				sue);

		type *builtin_ar = type_array_of(
				va_list_struct,
				e_one);

		decl *typedef_decl = decl_new_ty_sp(
				builtin_ar, ustrdup("__builtin_va_list"));

		expr *sz = expr_compiler_generated(expr_new_sizeof_type(builtin_ar, 1));

		sue_define(sue, sue_members);

		fold_decl_global(typedef_decl, symtab);
		FOLD_EXPR(sz, symtab);

		return type_tdef_of(sz, typedef_decl);
	}
}
