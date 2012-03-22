#ifndef ASM_OUT_H
#define ASM_OUT_H

typedef struct asm_output asm_output;
typedef struct asm_operand asm_operand;

typedef void        asm_out_func(    asm_output *);
typedef const char *asm_operand_func(asm_operand *);

/* functions */
asm_out_func
	asm_out_type_add,    asm_out_type_sub,   asm_out_type_imul,  asm_out_type_idiv,  asm_out_type_neg,
	asm_out_type_and,    asm_out_type_or,    asm_out_type_not,   asm_out_type_xor,
	asm_out_type_cmp,    asm_out_type_test,  asm_out_type_set,
	asm_out_type_jmp,    asm_out_type_call,
	asm_out_type_leave,  asm_out_type_ret,
	asm_out_type_mov,    asm_out_type_pop,   asm_out_type_push,  asm_out_type_lea,
	asm_out_type_shl,    asm_out_type_shr,
	asm_out_type_label;

/* "classes" */
struct asm_output
{
	asm_out_func *impl;

	asm_operand *lhs, *rhs;

	const char *extra; /* set%s */
};

struct asm_operand
{
	asm_operand_func *impl;

	decl *tt;

	/* reg */
	enum asm_reg
	{
		ASM_REG_A,  ASM_REG_B, ASM_REG_C, ASM_REG_D,
		ASM_REG_BP, ASM_REG_SP,
	} reg;

	/* label */
	const char *label;

	/* deref aka brackets */
	asm_operand *deref_base;
	int          deref_offset;

	/* immediate val */
	intval *iv;
};

asm_operand *asm_operand_new_reg(  decl *tree_type, enum asm_reg);
asm_operand *asm_operand_new_val(  int);
asm_operand *asm_operand_new_intval(intval *);
asm_operand *asm_operand_new_label(decl *tree_type, const char *lbl);
asm_operand *asm_operand_new_deref(decl *tree_type, asm_operand *deref_base, int offset);

asm_output *asm_output_new(
		asm_out_func *impl,
		asm_operand  *and_1,
		asm_operand  *and_2
	);

/* specialised */
void asm_set(const char *cmd, enum asm_reg);
void asm_jmp_if_zero(int invert, const char *lbl);
void asm_jmp(const char *lbl);
void asm_jmp_custom(const char *test, const char *lbl);
void asm_label(const char *);

void asm_comment(const char *, ...);

/* wrappers for asm_output_new */
void asm_push(enum asm_reg);
void asm_pop(decl *d, enum asm_reg); /* if decl isn't machine-word, truncations are done */

/* nothing to do with code */
void asm_out_section(enum section_type, const char *fmt, ...);

#define ASM_TEST(tt, reg)           \
	asm_output_new(                   \
			asm_out_type_test,            \
			asm_operand_new_reg(tt, reg), \
			asm_operand_new_reg(tt, reg))

void asm_flush(void);

#endif
