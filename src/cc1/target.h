#ifndef TARGET_H
#define TARGET_H

#include "../util/triple.h"

struct target_details
{
	struct target_section_names
	{
		const char *section_name_text;
		const char *section_name_data;
		const char *section_name_bss;
		const char *section_name_rodata;
		const char *section_name_ctors;
		const char *section_name_dtors;
		const char *section_name_dbg_abbrev;
		const char *section_name_dbg_info;
		const char *section_name_dbg_line;
	} section_names;

	struct target_as
	{
		struct
		{
			const char *weak;
			const char *visibility_hidden;
			const char *no_dead_strip;
		} directives;

		const char *privatelbl_prefix;

		int supports_visibility_protected;
		int supports_local_common;
	} as;

	int dwarf_indirect_section_links;
	int ld_indirect_call_via_plt;
};

void target_details_from_triple(const struct triple *, struct target_details *);

#endif
