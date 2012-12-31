#ifndef ASM_H
#define ASM_H

char        asm_type_ch(type_ref *);
const char *asm_type_directive(type_ref *);
void        asm_reg_name(type_ref *d, const char **regpre, const char **regpost);
int         asm_type_size(type_ref *);

void asm_out_section(enum section_type, const char *fmt, ...);

#define asm_declare_partial(...) asm_out_section(SECTION_DATA, __VA_ARGS__)

void asm_declare(FILE *f, decl *); /* x: .qword ... */

#endif
