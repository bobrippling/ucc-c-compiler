#ifndef VLA_H
#define VLA_H

unsigned vla_decl_space(decl *);
void vla_alloc_decl(decl *, out_ctx *);
const out_val *vla_address(decl *, out_ctx *);
const out_val *vla_size(decl *, out_ctx *);
const out_val *vla_gen_size(type *t, out_ctx *octx);

#endif
