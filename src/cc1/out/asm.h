#ifndef ASM_H
#define ASM_H

void asm_out_section(enum section_type, const char *fmt, ...);

void asm_declare_decl_init(FILE *f, decl *); /* x: .qword ... */

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
