#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

enum attribute_category
{
	attribute_cat_type = 1 << 0,

	attribute_cat_type_funconly = 1 << 1,
	attribute_cat_type_enumonly = 1 << 2,
	attribute_cat_type_enumentonly = 1 << 3,
	attribute_cat_type_ptronly = 1 << 4,
	attribute_cat_type_structonly = 1 << 5,

	attribute_cat_decl = 1 << 6,

	attribute_cat_decl_funconly = 1 << 7,
	attribute_cat_decl_varonly = 1 << 8,

	attribute_cat_label = 1 << 9,

	attribute_cat_any = ~0
};

/* - name/spelling + name,
 * - then an int describing whether the attribute
 *   takes part in type propagation
 * - then a category
 */

#define ATTRIBUTES                                                                                    \
		NAME(format, 0, attribute_cat_type_funconly)                                                      \
		NAME(unused, 0, attribute_cat_decl | attribute_cat_label)                                         \
		NAME(warn_unused, 0, attribute_cat_type_funconly)                                                 \
		NAME(section, 0, attribute_cat_decl)                                                              \
		NAME(enum_bitmask, 0, attribute_cat_type_enumonly)                                                \
		NAME(noreturn, 1, attribute_cat_type_funconly | attribute_cat_decl_funconly)                      \
		NAME(noderef, 1, attribute_cat_type_ptronly)                                                      \
		NAME(nonnull, 1, attribute_cat_type_funconly | attribute_cat_type_ptronly)                        \
		NAME(packed, 1, attribute_cat_type_structonly)                                                    \
		NAME(sentinel, 0, attribute_cat_type_funconly)                                                    \
		NAME(aligned, 1, attribute_cat_decl_varonly)                                                      \
		NAME(weak, 0, attribute_cat_decl)                                                                 \
		NAME(alias, 0, attribute_cat_decl)                                                                \
		NAME(cleanup, 0, attribute_cat_decl_varonly)                                                      \
		NAME(always_inline, 0, attribute_cat_decl_funconly)                                               \
		NAME(noinline, 0, attribute_cat_decl_funconly)                                                    \
		NAME(constructor, 0, attribute_cat_decl_funconly)                                                 \
		NAME(destructor, 0, attribute_cat_decl_funconly)                                                  \
		NAME(visibility, 0, attribute_cat_decl_funconly)                                                  \
		NAME(stack_protect, 0, attribute_cat_decl_funconly) /* gcc */                                     \
		NAME(no_stack_protector, 0, attribute_cat_decl_funconly) /* clang */                              \
		ALIAS("designated_init", desig_init, 0, attribute_cat_type_structonly)                            \
		ALIAS("__ucc_debug", ucc_debug, 0, attribute_cat_any) /* logs out a message when handled */       \
		ALIAS("__cdecl", call_conv, 0, attribute_cat_type_funconly)                                       \
		EXTRA_ALIAS("cdecl", call_conv, attribute_cat_type_funconly)                                      \
		EXTRA_ALIAS("stdcall", call_conv, attribute_cat_type_funconly)                                    \
		EXTRA_ALIAS("fastcall", call_conv, attribute_cat_type_funconly)                                   \
		EXTRA_ALIAS("warn_unused_result", warn_unused, attribute_cat_type_funconly | attribute_cat_label)

#endif
