#include <string.h>
#include <assert.h>

#include "../../util/dynmap.h"
#include "../../util/alloc.h"

#include "section.h"
#include "../cc1_target.h"

enum section_cmp_type
{
	BUILTIN,
	FUNCDATA,
	NAMED
};

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
	assert(sec->builtin != SECTION_UNINIT);
	switch(sec->builtin){
		case SECTION_FUNCDATA_FUNC:
		case SECTION_FUNCDATA_DATA:
			return 0;
		default:
			return !sec->name;
	}
}

char *section_name(const struct section *sec, int *const allocated)
{
	*allocated = 0;

	assert(sec->builtin != SECTION_UNINIT);

	if(sec->builtin == SECTION_FUNCDATA_FUNC || sec->builtin == SECTION_FUNCDATA_DATA){
		*allocated = 1;
		return ustrprintf(
				"%s.%s",
				sec->builtin == SECTION_FUNCDATA_FUNC
				? cc1_target_details.section_names.section_name_text
				: cc1_target_details.section_names.section_name_data,
				sec->name);
	}

	if(sec->name)
		return (char *)sec->name;

	assert(section_is_builtin(sec));
	switch(sec->builtin){
		case SECTION_TEXT: return (char *)cc1_target_details.section_names.section_name_text;
		case SECTION_DATA: return (char *)cc1_target_details.section_names.section_name_data;
		case SECTION_BSS: return (char *)cc1_target_details.section_names.section_name_bss;
		case SECTION_RODATA: return (char *)cc1_target_details.section_names.section_name_rodata;
		case SECTION_CTORS: return (char *)cc1_target_details.section_names.section_name_ctors;
		case SECTION_DTORS: return (char *)cc1_target_details.section_names.section_name_dtors;
		case SECTION_DBG_ABBREV: return (char *)cc1_target_details.section_names.section_name_dbg_abbrev;
		case SECTION_DBG_INFO: return (char *)cc1_target_details.section_names.section_name_dbg_info;
		case SECTION_DBG_LINE: return (char *)cc1_target_details.section_names.section_name_dbg_line;
	}

	assert(0 && "unreachable");
	return NULL;
}

static enum section_cmp_type to_type(const struct section *sec)
{
	assert(sec->builtin != SECTION_UNINIT);
	switch(sec->builtin){
		case SECTION_FUNCDATA_FUNC:
		case SECTION_FUNCDATA_DATA:
			return FUNCDATA;
		default:
			break;
	}
	if(sec->name)
		return NAMED;

	assert(sec->builtin >= 0);
	return BUILTIN;
}

int section_cmp(const struct section *a, const struct section *b)
{
	enum section_cmp_type a_type = to_type(a), b_type = to_type(b);

	if(a_type != b_type)
		return a_type - b_type;

	switch(a_type){
		case NAMED:
		case FUNCDATA: /* FUNCDATA compare differently to NAMED because of the above check */
			return strcmp(a->name, b->name);

		case BUILTIN:
			return a->builtin - b->builtin;
	}
	assert(0);
}

int section_eq(const struct section *a, const struct section *b)
{
	return !section_cmp(a, b);
}

int section_hash(const struct section *sec)
{
	enum section_cmp_type t = to_type(sec);
	switch(t){
		case BUILTIN:
		{
			int alloc;
			const char *str = section_name(sec, &alloc);
			assert(!alloc);
			return t ^ dynmap_strhash(str);
		}

		case FUNCDATA:
		case NAMED:
			return t ^ dynmap_strhash(sec->name);
	}
	assert(0);
}
