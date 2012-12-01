#ifndef IMPL_H
#define IMPL_H

extern struct machine_impl
{
	void (*store)(struct vstack *from, struct vstack *to);
	void (*load)(struct vstack *from, int reg);

	void (*reg_cp)(struct vstack *from, int r);
	void (*reg_swp)(struct vstack *a, struct vstack *b);

	void (*op)(enum op_type);
	void (*op_unary)(enum op_type); /* returns reg that the result is in */
	void (*deref)(void);
	void (*normalise)(void);

	void (*jmp)(void);
	void (*jcond)(int true, const char *lbl);

	void (*cast)(decl *from, decl *to);

	void (*call)(const int nargs, decl *d_ret, decl *d_func);

	int  (*alloc_stack)(int sz);

	void (*func_prologue)(int stack_res, int nargs, int variadic);
	void (*func_epilogue)(void);
	void (*pop_func_ret)(decl *);

	void (*undefined)(void);
	int  (*frame_ptr_to_reg)(int nframes);

	int (*n_regs)(void);
	int (*n_call_regs)(void);
} impl;

/* common */
extern int *reserved_regs;
extern int N_REGS, N_CALL_REGS;

enum p_opts
{
	P_NO_INDENT = 1 << 0,
	P_NO_NL     = 1 << 1
};

void out_asm( const char *fmt, ...) ucc_printflike(1, 2);
void out_asm2(enum p_opts opts, const char *fmt, ...) ucc_printflike(2, 3);

void impl_comment(const char *fmt, va_list l);
void impl_lbl(const char *lbl);

#endif
