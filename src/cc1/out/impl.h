#ifndef IMPL_H
#define IMPL_H

void impl_store(basic_blk *, struct vstack *from, struct vstack *to);
void impl_load(basic_blk *, struct vstack *from, const struct vreg *reg);
void impl_load_iv(basic_blk *, struct vstack *from);
void impl_load_fp(basic_blk *, struct vstack *from);

void impl_reg_cp(basic_blk *, struct vstack *from, const struct vreg *r);
void impl_reg_swp(basic_blk *, struct vstack *a, struct vstack *b);

void impl_op(basic_blk *, enum op_type);
void impl_op_unary(basic_blk *, enum op_type);
void impl_deref(basic_blk *,
		struct vstack *vp,
		const struct vreg *to,
		type_ref *tpointed_to);

//void impl_jmp(void);
//void impl_jcond(basic_blk *, int true, const char *lbl);

void impl_i2f(basic_blk *, struct vstack *, type_ref *t_i, type_ref *t_f);
void impl_f2i(basic_blk *, struct vstack *, type_ref *t_f, type_ref *t_i);
void impl_f2f(basic_blk *, struct vstack *, type_ref *from, type_ref *to);
void impl_cast_load(basic_blk *,
		struct vstack *vp,
		type_ref *small, type_ref *big,
		int is_signed);

void impl_call(basic_blk *, const int nargs, type_ref *r_ret, type_ref *r_func);
void impl_pop_func_ret(basic_blk *, type_ref *r);

void impl_func_prologue_save_fp(basic_blk *);

void impl_func_prologue_save_call_regs(
		basic_blk *,
		type_ref *rf, unsigned nargs,
		int arg_offsets[/*nargs*/]);

basic_blk *impl_func_prologue_save_variadic(basic_blk *, type_ref *rf);
void impl_func_epilogue(basic_blk *, type_ref *);

void impl_undefined(basic_blk *);
void impl_set_overflow(basic_blk *);
int  impl_frame_ptr_to_reg(basic_blk *, int nframes);
void impl_set_nan(basic_blk *, type_ref *);

/* scratch register indexing */
int  impl_reg_to_scratch(const struct vreg *);
void impl_scratch_to_reg(int scratch, struct vreg *);

/* callee save register bools */
int impl_reg_is_callee_save(const struct vreg *r, type_ref *fr);

enum p_opts
{
	P_NO_INDENT = 1 << 0,
	P_NO_NL     = 1 << 1
};

void out_asm(basic_blk *, const char *fmt, ...) ucc_printflike(2, 3);

void impl_comment_sec(enum section_type, const char *fmt, va_list l);
void impl_lbl(const char *lbl);

enum flag_cmp op_to_flag(enum op_type op);

/* can't do this for gen_deps.sh */
#ifdef CC1_IMPL_FNAME
#  include CC1_IMPL_FNAME
#else
#  warning "no impl defined"
#endif

#ifndef MIN
#  define MIN(x, y) ((x) < (y) ? (x) : (y))
#  define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#endif
