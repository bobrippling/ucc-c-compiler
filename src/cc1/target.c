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
		"__DATA,__const",
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
		".ctors,\"w\"",
		".dtors,\"w\"",
		".debug_abbrev",
		".debug_info",
		".debug_line",
	},
};

static const struct target_as asconfig[] = {
	{
		{
			"weak",
			"hidden",
		},
		".L",
		1, /* visibility protected */
		1, /* local common */
		1, /* stackprotector via tls */
	},
	{
		{
			"weak",
			"hidden",
		},
		".L",
		1, /* visibility protected */
		1, /* local common */
		1, /* stackprotector via tls */
	},
	{
		{
			"weak_reference", /* Darwin also needs "-flat_namespace -undefined suppress" */
			"private_extern",
		},
		"L",
		0, /* visibility protected */
		0, /* local common */
		0, /* stackprotector via tls */
	},
	{
		{
			"weak",
			"hidden",
		},
		".L",
		1, /* visibility protected */
		1, /* local common */
		1, /* stackprotector via tls */
	},
};

static const int dwarf_indirect_section_linkss[] = {
	0,
	0,
	1,
	0,
};

static const int ld_indirect_call_via_plts[] = {
	1,
	1,
	0,
	1,
};

ucc_unused
static char syses[] = {
#define X(pre, post) 0,
#define X_ncmp(pre, post, n) X(pre, post)
	TARGET_SYSES
#undef X
#undef X_ncmp
};

ucc_static_assert(size_match1, countof(syses) == countof(section_names));
ucc_static_assert(size_match2, countof(syses) == countof(asconfig));
ucc_static_assert(size_match3, countof(syses) == countof(dwarf_indirect_section_linkss));
ucc_static_assert(size_match4, countof(syses) == countof(ld_indirect_call_via_plts));

void target_details_from_triple(const struct triple *triple, struct target_details *details)
{
	memcpy(&details->section_names, &section_names[triple->sys], sizeof(details->section_names));
	memcpy(&details->as, &asconfig[triple->sys], sizeof(details->as));

	details->dwarf_indirect_section_links = dwarf_indirect_section_linkss[triple->sys];
	details->ld_indirect_call_via_plt = ld_indirect_call_via_plts[triple->sys];
}
