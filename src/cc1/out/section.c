#include <string.h>
#include <assert.h>

#include "../../util/dynmap.h"

#include "section.h"
#include "../cc1_target.h"

const struct section section_text = SECTION_INIT(SECTION_TEXT);
const struct section section_data = SECTION_INIT(SECTION_DATA);
const struct section section_bss = SECTION_INIT(SECTION_BSS);
const struct section section_rodata = SECTION_INIT(SECTION_RODATA);
const struct section section_ctors = SECTION_INIT(SECTION_CTORS);
const struct section section_dtors = SECTION_INIT(SECTION_DTORS);
const struct section section_dbg_abbrev = SECTION_INIT(SECTION_DBG_ABBREV);
const struct section section_dbg_info = SECTION_INIT(SECTION_DBG_INFO);
const struct section section_dbg_line = SECTION_INIT(SECTION_DBG_LINE);

int section_is_builtin(const struct section *sec)
{
	return !sec->name;
}

static int is_named(const struct section *sec)
{
	return !is_builtin(sec);
}

const char *section_name(const struct section *sec)
{
	if(is_named(sec))
		return sec->name;

	switch(sec->builtin){
		case SECTION_TEXT: return cc1_target_details.section_names.section_name_text;
		case SECTION_DATA: return cc1_target_details.section_names.section_name_data;
		case SECTION_BSS: return cc1_target_details.section_names.section_name_bss;
		case SECTION_RODATA: return cc1_target_details.section_names.section_name_rodata;
		case SECTION_CTORS: return cc1_target_details.section_names.section_name_ctors;
		case SECTION_DTORS: return cc1_target_details.section_names.section_name_dtors;
		case SECTION_DBG_ABBREV: return cc1_target_details.section_names.section_name_dbg_abbrev;
		case SECTION_DBG_INFO: return cc1_target_details.section_names.section_name_dbg_info;
		case SECTION_DBG_LINE: return cc1_target_details.section_names.section_name_dbg_line;
	}

	assert(0 && "unreachable");
	return NULL;
}

int section_cmp(const struct section *a, const struct section *b)
{
	int named_a = is_named(a);
	int named_b = is_named(b);

	if(named_a != named_b)
		return strcmp(section_name(a), section_name(b));

	if(named_a)
		return strcmp(a->name, b->name);

	if(a->builtin == b->builtin)
		return 0;

	return a->builtin - b->builtin;
}

int section_eq(const struct section *a, const struct section *b)
{
	return !section_cmp(a, b);
}

int section_hash(const struct section *sec)
{
	return dynmap_strhash(section_name(sec));
}
