#ifndef OUT_ALTSTACK_H
#define OUT_ALTSTACK_H

void v_altstack_pop_all(out_ctx *octx);
void v_altstack_push_all(out_ctx *octx);

void v_altstack_reserve(out_ctx *);
void v_altstack_lock(const out_val *);
void v_altstack_unlock(const out_val *);

#endif
