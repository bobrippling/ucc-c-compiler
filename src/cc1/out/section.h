#ifndef OUT_SECTION_H
#define OUT_SECTION_H

enum section_builtin
{
	SECTION_TEXT,
	SECTION_DATA,
	SECTION_BSS,
	SECTION_RODATA,
	SECTION_CTORS,
	SECTION_DTORS,
	SECTION_DBG_ABBREV,
	SECTION_DBG_INFO,
	SECTION_DBG_LINE
};

struct section
{
	const char *name; /* used if present, else builtin */
	int builtin;
};

#define SECTION_BEGIN "section_begin_"
#define SECTION_END   "section_end_"
#define SECTION_DESC_TEXT "text"
#define SECTION_DESC_DATA "data"
#define SECTION_DESC_BSS "bss"
#define SECTION_DESC_RODATA "rodata"
#define SECTION_DESC_CTORS "ctors"
#define SECTION_DESC_DTORS "dtors"
#define SECTION_DESC_DBG_ABBREV "dbg_abbrev"
#define SECTION_DESC_DBG_INFO "dbg_info"
#define SECTION_DESC_DBG_LINE "dbg_line"

#define SECTION_FROM_BUILTIN(sec, sbuiltin) do{ \
	(sec)->builtin = (sbuiltin); \
	(sec)->name = NULL; \
}while(0)

#define SECTION_FROM_NAME(sec, str) do{ \
	(sec)->name = (str); \
	(sec)->builtin = 0; \
}while(0)

#define SECTION_INIT(builtin) { NULL, builtin }

#define SECTION_IS_BUILTIN(sec) (!(sec)->name)

const char *section_name(const struct section *);
int section_eq(const struct section *, const struct section *);

extern const struct section section_text;
extern const struct section section_data;
extern const struct section section_bss;
extern const struct section section_rodata;
extern const struct section section_ctors;
extern const struct section section_dtors;
extern const struct section section_dbg_abbrev;
extern const struct section section_dbg_info;
extern const struct section section_dbg_line;

#endif
