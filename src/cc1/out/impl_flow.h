#ifndef IMPL_FLOW_H
#define IMPL_FLOW_H

void impl_lbl(FILE *, const char *);

void impl_flag_or_const(struct vstack *vp, struct basic_blk *bb);

void impl_jmp(FILE *, const char *lbl);
void impl_jflag(
		FILE *, struct vstack_flag *flag,
		const char *ltrue, const char *lfalse);

#endif
