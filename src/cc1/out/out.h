#ifndef OUT_H
#define OUT_H

enum out_label_type
{
	CASE_CASE,
	CASE_DEF,
	CASE_RANGE
};

void out_pop(void);

void out_push_iv(decl *d, intval *iv);
void out_push_i( decl *d, int i);
void out_push_lbl(const char *); /* implicitly pointer */

void out_dup(void); /* duplicate top of stack */

void out_push_sym_addr(sym *);
void out_push_sym(sym *);
void out_store(decl *d); /* store `pop` into *`pop` */

void out_op(enum op_type, decl *d); /* both binary ops, unary ops, and comparisons */

void out_cast(decl *from, decl *to);

void out_call(void); /* call *pop(), push result */
void out_call_fin(int); /* remove args from stack */
void out_jmp( void); /* jmp to *pop() */
void out_jz( const char *);
void out_jnz(const char *);

void out_func_prologue(int offset); /* push rbp, sub rsp, ... */
void out_func_epilogue(void); /* mov rsp, rbp; ret */
void out_label(const char *);

void out_comment(const char *, ...);

void out_flush(void);

/* nothing to do with code */
void asm_out_section(enum section_type, const char *fmt, ...);

char *out_label_code(const char *fmt);
char *out_label_array(int str);
char *out_label_static_local(const char *funcsp, const char *spel);
char *out_label_goto(char *lbl);
char *out_label_case(enum out_label_type, int val);
char *out_label_flow(const char *fmt);
char *out_label_block(const char *funcsp);

#endif
