#ifndef ASM_H
#define ASM_H

enum section_type
{
	SECTION_TEXT,
	SECTION_DATA,
	SECTION_BSS,
	NUM_SECTIONS
};

extern FILE *cc_out[NUM_SECTIONS];
extern FILE *cc1_out;

void asm_out_section(enum section_type, const char *fmt, ...);
void asm_out_sectionv(enum section_type t, const char *fmt, va_list l);

void asm_nam_begin3(enum section_type sec, const char *lbl, unsigned align);

void asm_out_fp(enum section_type sec, type_ref *ty, floating_t f);

void asm_declare_stringlit(enum section_type, const stringlit *);

void asm_declare_decl_init(enum section_type, decl *); /* x: .qword ... */

void asm_predeclare_extern(decl *d);
void asm_predeclare_global(decl *d);

/* in impl */
extern const struct asm_type_table
{
	int sz;
	const char *directive;
} asm_type_table[];
#define ASM_TABLE_LEN 4

int         asm_table_lookup(type_ref *);
int         asm_type_size(type_ref *);
const char *asm_type_directive(type_ref *);

#endif
