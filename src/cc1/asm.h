#ifndef ASM_H
#define ASM_H

enum asm_label_type
{
	CASE_CASE,
	CASE_DEF,
	CASE_RANGE
};

char        asm_type_ch(decl *);
const char *asm_type_str(decl *);
const char *asm_reg_name(decl *);
int         asm_type_size(decl *);

void asm_label(const char *);

void asm_sym_load(     sym *, const char *reg);
void asm_sym_store(    sym *, const char *reg);
void asm_sym_load_addr(sym *, const char *reg);

void asm_declare_array(enum section_type output, const char *lbl, array_decl *ad);
void asm_declare_single_part(FILE *f, expr *e);
void asm_declare_single(     FILE *f, decl *d);

char *asm_label_code(const char *fmt);
char *asm_label_array(int str);
char *asm_label_static_local(const char *funcsp, const char *spel);
char *asm_label_goto(char *lbl);
char *asm_label_case(enum asm_label_type, int val);
char *asm_label_flowfin(void);
#define asm_label_break(flow_t) flow_t->lblfin

#endif
