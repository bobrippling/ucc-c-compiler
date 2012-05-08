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

enum asm_size
{
	ASM_SIZE_WORD,
	ASM_SIZE_1,
	ASM_SIZE_STRUCT_UNION /* special case */
};

enum asm_indir
{
	ASM_INDIR_GET,
	ASM_INDIR_SET
};

void asm_new(enum asm_type, void *);

void asm_temp(          int indent, const char *, ...);
void asm_tempf(FILE *f, int indent, const char *, ...);
void asm_out_intval(FILE *f, intval *iv);

void asm_indir(enum asm_indir mode, decl *tt, char rto, char rfrom, const char *comment);

void asm_label(const char *);
void asm_sym(enum asm_sym_type, sym *, const char *);

char asm_type_ch(decl *d);
enum asm_size asm_type_size(decl *d);

void asm_declare_array(enum section_type output, const char *lbl, array_decl *ad);
void asm_declare_single(FILE *f, decl *d);
void asm_declare_single_part(FILE *f, expr *e);

char *asm_label_code(const char *fmt);
char *asm_label_array(int str);
char *asm_label_static_local(const char *funcsp, const char *spel);
char *asm_label_goto(char *lbl);
char *asm_label_case(enum asm_label_type, int val);
char *asm_label_flow(const char *fmt);

#endif
