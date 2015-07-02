// --------- type warnings

struct A
{
	int i __attribute((unused)); // warn
};

struct A __attribute((packed)) a; // warn

__attribute((format(printf,1,2))) int glob; // warn

__attribute((format(printf,1,2))) int f(char*,...);

typedef int i __attribute((unused)); // warn

__attribute((warn_unused_result)) int g();

__attribute((warn_unused_result)) char *p; // warn

int b __attribute((enum_bitmask)); // warn

char *__attribute((enum_bitmask)) str = "hi"; // warn

void __attribute((noreturn)) *noret_x; // warn

__attribute((noderef)) char c; // warn
__attribute((noderef)) char *cp; // fine - odd usage but accepted
char *__attribute((noderef)) cp2;


/*
NAME(noderef, attribute_cat_type_ptronly)                                                         \
NAME(nonnull, attribute_cat_type_funconly | attribute_cat_type_ptronly)                           \
NAME(packed, attribute_cat_type_structonly)                                                       \
NAME(sentinel, attribute_cat_type_funconly)                                                       \
ALIAS("designated_init", desig_init, attribute_cat_type_structonly)                               \
ALIAS("__ucc_debug", ucc_debug, attribute_cat_any)
ALIAS("__cdecl", call_conv, attribute_cat_type_funconly)                                          \
EXTRA_ALIAS("cdecl", call_conv, attribute_cat_type_funconly)                                      \
EXTRA_ALIAS("stdcall", call_conv, attribute_cat_type_funconly)                                    \
EXTRA_ALIAS("fastcall", call_conv, attribute_cat_type_funconly)                                   \
EXTRA_ALIAS("warn_unused_result", warn_unused, attribute_cat_type_funconly | attribute_cat_label)
*/

// --------- decl warnings

int x __attribute((section("yo")));

/*
NAME(section, attribute_cat_decl)                                                                 \
NAME(aligned, attribute_cat_decl_varonly)                                                         \
NAME(weak, attribute_cat_decl)                                                                    \
NAME(cleanup, attribute_cat_decl_varonly)                                                         \
NAME(always_inline, attribute_cat_decl_funconly)                                                  \
NAME(noinline, attribute_cat_decl_funconly)                                                       \
*/
