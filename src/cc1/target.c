#include <string.h>

#include "target.h"
#include "../util/macros.h"
#include "../util/compiler.h"

static const struct target_section_names section_names[] = {
	{
		".text",
		".data",
		".bss",
		".rodata",
		".data.rel.ro", /* .data.rel.ro[.local] */
		".init_array,\"aw\"",
		".fini_array,\"aw\"",
		".debug_abbrev",
		".debug_info",
		".debug_line",
	},
	{
		".text",
		".data",
		".bss",
		".rodata",
		".data.rel.ro", /* .data.rel.ro[.local] */
		".init_array,\"aw\"",
		".fini_array,\"aw\"",
		".debug_abbrev",
		".debug_info",
		".debug_line",
	},
	{
		"__TEXT,__text",
		"__DATA,__data",
		"__BSS,__bss",
		"__TEXT,__const", /* no relocs required (oddly text) */
		"__DATA,__const", /* relocs required */
		"__DATA,__mod_init_func,mod_init_funcs",
		"__DATA,__mod_term_func,mod_term_funcs",
		"__DWARF,__debug_abbrev,regular,debug",
		"__DWARF,__debug_info,regular,debug",
		"__DWARF,__debug_line,regular,debug",
	},
	{
		".text",
		".data",
		".bss",
		".rodata",
		".data.rel.ro",
		".ctors,\"w\"",
		".dtors,\"w\"",
		".debug_abbrev",
		".debug_info",
		".debug_line",
	},
};

static const struct target_as toolchain_gnu = {
	{
		"weak",
		"hidden",
		NULL,
	},
	".L",
	1, /* visibility protected */
	1, /* local common */
	1, /* stackprotector via tls */
	1, /* supports_type_and_size */
	1, /* supports_section_flags */
	1, /* expr inline */
};

static const struct target_as toolchain_darwin = {
	{
		"weak_reference", /* Darwin also needs "-flat_namespace -undefined suppress" */
		"private_extern",
		"no_dead_strip",
	},
	"L",
	0, /* visibility protected */
	0, /* local common */
	0, /* stackprotector via tls */
	0, /* supports_type_and_size */
	0, /* supports_section_flags */
	0, /* expr inline */
};

static const struct target_as *const asconfig[] = {
	&toolchain_gnu,    /* linux */
	&toolchain_gnu,    /* freebsd */
	&toolchain_darwin, /* darwin */
	&toolchain_gnu,    /* cygwin */
};

static const int dwarf_link_stmt_list[] = {
	1, /* linux */
	1, /* freebsd */
	0, /* darwin */
	1, /* cygwin */
};

static const int ld_indirect_call_via_plts[] = {
	1, /* linux */
	1, /* freebsd */
	0, /* darwin */
	1, /* cygwin */
};

static const int alias_variables[] = {
	1, /* linux */
	1, /* freebsd */
	0, /* darwin */
	1, /* cygwin */
};

static const int dtor_via_ctor_atexit[] = {
	0, /* linux */
	0, /* freebsd */
	1, /* darwin */
	0, /* cygwin */
};

ucc_unused
static char syses[] = {
#define X(pre, post) 0,
#define X_ncmp(pre, post, n) X(pre, post)
#define X_ncmp_alias(pre, target, alias, n)
	TARGET_SYSES
#undef X
#undef X_ncmp
#undef X_ncmp_alias
};

ucc_static_assert(size_match1, countof(syses) == countof(section_names));
ucc_static_assert(size_match2, countof(syses) == countof(asconfig));
ucc_static_assert(size_match3, countof(syses) == countof(dwarf_link_stmt_list));
ucc_static_assert(size_match4, countof(syses) == countof(ld_indirect_call_via_plts));
ucc_static_assert(size_match5, countof(syses) == countof(alias_variables));

void target_details_from_triple(const struct triple *triple, struct target_details *details)
{
	details->section_names = &section_names[triple->sys];
	details->as = asconfig[triple->sys];

	details->dwarf_link_stmt_list = dwarf_link_stmt_list[triple->sys];
	details->ld_indirect_call_via_plt = ld_indirect_call_via_plts[triple->sys];
	details->alias_variables = alias_variables[triple->sys];
	details->dtor_via_ctor_atexit = dtor_via_ctor_atexit[triple->sys];
}
