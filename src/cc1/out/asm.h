#ifndef ASM_H
#define ASM_H

char        asm_type_ch(type_ref *);
const char *asm_type_directive(type_ref *);
void        asm_reg_name(type_ref *d, const char **regpre, const char **regpost);
int         asm_type_size(type_ref *);

const char *asm_intval_str(intval *iv);

void asm_out_section(enum section_type, const char *fmt, ...);

void asm_declare(FILE *f, decl *); /* x: .qword ... */
void asm_declare_partial(const char *, ...); /* .qword ... */

#endif
