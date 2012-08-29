#ifndef ASM_H
#define ASM_H

char        asm_type_ch(decl *);
const char *asm_type_directive(decl *);
void        asm_reg_name(decl *d, const char **regpre, const char **regpost);
int         asm_type_size(decl *);

const char *asm_intval_str(intval *iv);

void asm_out_section(enum section_type, const char *fmt, ...);

/* declare 'd' */
void asm_declare_partial(const char *, ...);

/* declare array */
void asm_declare_array(const char *lbl, array_decl *ad);

/* .bss */
void asm_reserve_bytes(const char *lbl, int nbytes);

#endif
