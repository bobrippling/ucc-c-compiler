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
	const char *name;
	int builtin;
	/*
	 * if(builtin == SECTION_UNINIT), this is uninitialised,
	 * if(builtin == SECTION_FUNC/DATA), this is a function/data sec with .name being the spel
	 * if(name), this is a custom named section
	 * else, this is a builtin section
	 */

	/* used for ELF targets to emit section flags */
	enum section_flags {
		SECTION_FLAG_EXECUTABLE = 1 << 0,
		SECTION_FLAG_RO = 1 << 1,
	} flags;
};

#define SECTION_UNINIT -1
#define SECTION_FUNCDATA_FUNC -2
#define SECTION_FUNCDATA_DATA -3

#define SECTION_INIT(builtin) { NULL, builtin, -1 }

#define SECTION_FROM_BUILTIN(sec, sbuiltin, flags) do{ \
	(sec)->builtin = (sbuiltin); \
	(sec)->name = NULL; \
	(sec)->flags = flags; \
}while(0)

#define SECTION_FROM_NAME(sec, str, flags) do{ \
	(sec)->name = (str); \
	(sec)->builtin = 0; \
	(sec)->flags = flags; \
}while(0)

#define SECTION_FROM_FUNCDECL(sec, spel, flags) do{ \
	(sec)->name = (spel); \
	(sec)->builtin = SECTION_FUNCDATA_FUNC; \
	(sec)->flags = flags; \
}while(0)

#define SECTION_FROM_DATADECL(sec, spel, flags) do{ \
	(sec)->name = (spel); \
	(sec)->builtin = SECTION_FUNCDATA_DATA; \
	(sec)->flags = flags; \
}while(0)

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

char *section_name(const struct section *, int *const allocated);
int section_eq(const struct section *, const struct section *);

int section_hash(const struct section *);
int section_cmp(const struct section *, const struct section *);

int section_is_builtin(const struct section *);

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
