#ifndef IMPL_FLOW_H
#define IMPL_FLOW_H

void impl_lbl(FILE *, const char *);
void impl_jmp(basic_blk *, const char *lbl);

void impl_jcond_make(
		struct basic_blk_fork *b_fork,
		struct basic_blk *bb,
		const char *tlbl, const char *flbl);

void impl_jflag_make(
		struct basic_blk_fork *b_fork,
		struct flag_opts *flag,
		const char *ltrue, const char *lfalse);

#endif
