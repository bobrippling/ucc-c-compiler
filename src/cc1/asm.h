#ifndef ASM_H
#define ASM_H

typedef struct
{
	enum asm_type
	{
		asm_assign,
		asm_call,
		asm_load_ident,
		asm_load_val,
		asm_op,
		asm_pop,
		asm_push,
		asm_addrof
	} type;

} asmop;

enum asm_sym_type
{
	ASM_SET,
	ASM_LOAD,
	ASM_LEA
};

enum asm_label_type
{
	CASE_CASE,
	CASE_DEF,
	CASE_RANGE
};

void asm_new(enum asm_type, void *);
void asm_temp(          int indent, const char *, ...);
void asm_tempf(FILE *f, int indent, const char *, ...);
void asm_label(const char *);
void asm_sym(enum asm_sym_type, sym *, const char *);
void asm_sym_struct(expr *, const char *);

int asm_type_ch(decl *d);

void asm_declare_array(enum section_type output, const char *lbl, array_decl *ad);
void asm_declare_single(FILE *f, decl *d);
void asm_declare_single_part(FILE *f, expr *e);

char *asm_label_code(const char *fmt);
char *asm_label_array(int str);
char *asm_label_static_local(decl *df, const char *spel);
char *asm_label_goto(char *lbl);
char *asm_label_case(enum asm_label_type, int val);
#define asm_label_break(flow_t) flow_t->lblfin
char *asm_label_flowfin(void);

#endif
