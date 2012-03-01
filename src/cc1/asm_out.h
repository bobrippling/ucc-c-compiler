#ifndef ASM_OUT_H
#define ASM_OUT_H

typedef struct asm_output asm_output;
typedef struct asm_operand asm_operand;

typedef void        (*asm_out_fptr    )(asm_output *);
typedef const char *(*asm_operand_fptr)(asm_operand *);

/* functions */
asm_out_fptr
	asm_out_type_add,    asm_out_type_sub,   asm_out_type_imul,  asm_out_type_idiv,  asm_out_type_neg,
	asm_out_type_and,    asm_out_type_or,    asm_out_type_not,   asm_out_type_xor,
	asm_out_type_cmp,    asm_out_type_test,  asm_out_type_set,
	asm_out_type_jmp,    asm_out_type_call,
	asm_out_type_leave,  asm_out_type_ret,
	asm_out_type_mov,    asm_out_type_pop,   asm_out_type_push;

asm_operand_fptr asm_operand_reg, asm_operand_label, asm_operand_deref; 

/* "classes" */
struct asm_output
{
	asm_out_fptr impl;

	asm_operand *lhs, *rhs;

	const char *label_jump;
#define label_call label_jump
#define set_mode   label_jump
};

struct asm_operand
{
	asm_operand_fptr impl;

	/* reg */
	char reg;

	/* label */
	const char *label;

	/* deref aka brackets */
	asm_operand *deref_base;
	int          deref_offset;
};

asm_operand *asm_operand_new_reg(  decl *tree_type, char reg);
asm_operand *asm_operand_new_val(  decl *tree_type, int val);
asm_operand *asm_operand_new_label(decl *tree_type, const char *lbl);
asm_operand *asm_operand_new_deref(decl *tree_type, asm_operand *deref_base, int offset);

void asm_output_new(
		asm_out_fptr  impl,
		asm_operand  *and_1,
		asm_operand  *and_2
	);


void asm_comment(const char *, ...);

/* wrappers for asm_output_new */
void asm_push(char reg);
void asm_pop( char reg);

#endif
