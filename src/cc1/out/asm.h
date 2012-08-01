#ifndef ASM_H
#define ASM_H

char        asm_type_ch(decl *);
const char *asm_type_directive(decl *);
void        asm_reg_name(decl *d, const char **regpre, const char **regpost);
int         asm_type_size(decl *);

const char *asm_intval_str(intval *iv);

void asm_declare_out(FILE *f, decl *d, const char *fmt, ...);
void asm_declare_array(const char *lbl, array_decl *ad);
void asm_declare_single_part(expr *e);
void asm_declare_single(     decl *d);
void asm_reserve_bytes(const char *lbl, int nbytes);

#endif
