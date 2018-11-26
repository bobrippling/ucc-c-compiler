#ifndef OUT_CTRL_H
#define OUT_CTRL_H

out_val *out_val_blockphi_make(out_ctx *, const out_val *, out_blk *);
out_val *out_val_unphi(out_ctx *, const out_val *phival);
int out_val_is_blockphi(const out_val *, out_blk * /*optional*/);

#endif
