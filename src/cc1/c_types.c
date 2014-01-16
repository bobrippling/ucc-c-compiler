#include "c_types.h"

type *c_types_make_va_list()
{
	/* pointer to struct __builtin_va_list */
	/* must match platform abi - vfprintf(..., ap); */
	sue_member **sue_members = NULL;

	type *void_ptr = type_cached_VOID_PTR();

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
	      type_new_type(             \
	        type_new_primitive(ty)), \
	      ustrdup(sp)))


	ADD_SCALAR(sue_members, type_int, "gp_offset");
	ADD_SCALAR(sue_members, type_int, "fp_offset");
	ADD_DECL(sue_members, decl_new_ty_sp(void_ptr, "overflow_arg_area"));
	ADD_DECL(sue_members, decl_new_ty_sp(void_ptr, "reg_save_area"));

	/* typedef struct __va_list_struct __builtin_va_list[1]; */
	{
		type *va_list_struct = type_new_type(
				type_new_primitive_sue(
					type_struct,
					sue_decl(stab, ustrdup("__va_list_struct"),
						sue_members, type_struct, 1, 1)));


		type *builtin_ar = type_new_array(
				va_list_struct,
				expr_new_val(1));

		type *td = type_new_tdef(
				expr_new_sizeof_type(builtin_ar, 1),
				decl_new_ty_sp(builtin_ar,
					ustrdup("__builtin_va_list")));

		cache_va_list = td;
	}
}
