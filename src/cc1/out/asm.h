#ifndef ASM_H
#define ASM_H

char        asm_type_ch(decl *);
const char *asm_type_directive(decl *);
void        asm_reg_name(decl *d, const char **regpre, const char **regpost);
int         asm_type_size(decl *);

const char *asm_intval_str(intval *iv);

void asm_out_section(enum section_type, const char *fmt, ...);

void asm_declare(FILE *f, decl *d); /* x: .qword ... */
void asm_declare_partial(const char *, ...); /* .qword ... */

#endif
