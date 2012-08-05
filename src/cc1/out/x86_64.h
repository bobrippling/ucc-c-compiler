#ifndef X86_64_H
#define X86_64_H

void impl_store(int reg, struct vstack *where);
int  impl_load(struct vstack *value); /* returns register index */

void impl_comment(const char *, va_list);

#endif
