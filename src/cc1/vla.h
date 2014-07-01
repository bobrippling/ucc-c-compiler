#ifndef VLA_H
#define VLA_H

unsigned vla_space(void);
long vla_ptr_offset(long start);
void vla_alloc(decl *, out_ctx *);
const out_val *vla_address(decl *, out_ctx *);
const out_val *vla_size(decl *, out_ctx *);
const out_val *vla_gen_size(type *t, out_ctx *octx);

#endif
