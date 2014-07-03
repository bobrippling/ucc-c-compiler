#ifndef VLA_H
#define VLA_H

/* space required for a vla */
unsigned vla_decl_space(decl *);

/* allocate and initialise space for a decl and all it sub-type sizes */
void vla_alloc_decl(decl *, out_ctx *);

/* get the address of the vla pointer */
const out_val *vla_address(decl *, out_ctx *);

/* saved stack pointer */
const out_val *vla_saved_ptr(decl *, out_ctx *);

/* get the size of a VM - pull from the cached location
 * if present (created by vla_alloc_decl()). Otherwise
 * it evaluates side effects of the type
 *
 * int x[expr]; sizeof(x) -> cached expr/size
 * sizeof(T[n]) -> no cached expr/size
 */
const out_val *vla_size(type *, out_ctx *);

#endif
