#ifndef ASM_H
#define ASM_H

enum asm_label_type
{
	CASE_CASE,
	CASE_DEF,
	CASE_RANGE
};

enum asm_sym_type
{
	ASM_LEA,
	ASM_LOAD,
	ASM_STORE
};

char        asm_type_ch(decl *);
const char *asm_type_directive(decl *);
void        asm_reg_name(decl *d, const char **regpre, const char **regpost);
int         asm_type_size(decl *);

const char *asm_intval_str(intval *iv);

void asm_sym(enum asm_sym_type, sym *, asm_operand *reg);

void asm_declare_out(FILE *f, decl *d, const char *fmt, ...);
void asm_declare_array(const char *lbl, array_decl *ad);
void asm_declare_single_part(expr *e);
void asm_declare_single(     decl *d);
void asm_reserve_bytes(const char *lbl, int nbytes);

char *asm_label_code(const char *fmt);
char *asm_label_array(int str);
char *asm_label_static_local(const char *funcsp, const char *spel);
char *asm_label_goto(char *lbl);
char *asm_label_case(enum asm_label_type, int val);
char *asm_label_flow(const char *fmt);
char *asm_label_block(const char *funcsp);

#endif
