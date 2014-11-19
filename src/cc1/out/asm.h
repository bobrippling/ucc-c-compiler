#ifndef OUT_ASM_H
#define OUT_ASM_H

enum section_type
{
	SECTION_TEXT,
	SECTION_DATA,
	SECTION_BSS,
	SECTION_RODATA,
	SECTION_DBG_ABBREV,
	SECTION_DBG_INFO,
	SECTION_DBG_LINE,
	NUM_SECTIONS
};

#define SECTION_BEGIN ASM_PLBL_PRE "section_begin_"
#define SECTION_END   ASM_PLBL_PRE "section_end_"
#define QUOTE_(...) #__VA_ARGS__
#define QUOTE(y) QUOTE_(y)

extern struct section
{
	const char *desc;
	const char *name;
} sections[];

extern FILE *cc_out[NUM_SECTIONS];
extern FILE *cc1_out;

void asm_out_section(enum section_type, const char *fmt, ...);
void asm_out_sectionv(enum section_type t, const char *fmt, va_list l);

int asm_section_empty(enum section_type);

void asm_nam_begin3(enum section_type sec, const char *lbl, unsigned align);

void out_comment_sec(enum section_type sec, const char *fmt, ...);

#ifdef TYPE_H
void asm_out_fp(enum section_type sec, type *ty, floating_t f);
void asm_out_fp(enum section_type sec, type *ty, floating_t f);
#endif

#ifdef STRINGS_H
void asm_declare_stringlit(enum section_type, const stringlit *);
#endif

#ifdef DECL_H
void asm_declare_decl_init(decl *); /* x: .qword ... */

void asm_predeclare_extern(decl *d);
void asm_predeclare_global(decl *d);
void asm_predeclare_weak(decl *d);
#endif

/* in impl */
extern const struct asm_type_table
{
	int sz;
	const char *directive;
} asm_type_table[];
#define ASM_TABLE_LEN 4

#ifdef TYPE_H
int         asm_table_lookup(type *);
int         asm_type_size(type *);
const char *asm_type_directive(type *);
#endif

#endif
