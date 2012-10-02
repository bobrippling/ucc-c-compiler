#ifndef IMPL_H
#define IMPL_H

extern struct machine_impl
{
	void (*comment)(const char *, va_list);

	void (*store)(struct vstack *from, struct vstack *to);
	void (*load)(struct vstack *from, int reg);

	void (*reg_cp)(struct vstack *from, int r);

	void (*op)(enum op_type);
	void (*op_unary)(enum op_type); /* returns reg that the result is in */
	void (*deref)(void);
	void (*normalise)(void);

	void (*jmp)(void);
	void (*jcond)(int true, const char *lbl);

	void (*cast)(decl *from, decl *to);

	void (*call)(const int nargs, decl *d_ret, decl *d_func);
	void (*call_fin)(int nargs);

	void (*lbl)(const char *);

	int  (*alloc_stack)(int sz);

	void (*func_prologue)(int stack_res, int nargs, int variadic);
	void (*func_epilogue)(void);
	void (*pop_func_ret)(decl *);

	void (*undefined)(void);
	int  (*frame_ptr_to_reg)(int nframes);

	int (*n_regs)(void);
	int (*n_call_regs)(void);
} impl;

#endif
