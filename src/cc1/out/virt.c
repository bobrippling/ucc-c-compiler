void v_stack_adj(unsigned amt, int sub)
{
	v_push_sp();
	out_push_l(type_nav_btype(cc1_type_nav, type_intptr_t), amt);
	out_op(sub ? op_minus : op_plus);
	out_flush_volatile();
	out_pop();
}

unsigned v_alloc_stack2(
		const unsigned sz_initial, int noop, const char *desc)
{
	unsigned sz_rounded = sz_initial;

	if(sz_initial){
		/* must be a multiple of mstack_align.
		 * assume stack_sz is aligned, and just
		 * align what we add to it
		 */
		sz_rounded = pack_to_align(sz_initial, cc1_mstack_align);

		/* if it changed, we need to realign the stack */
		if(!noop || sz_rounded != sz_initial){
			unsigned to_alloc;

			if(!noop){
				to_alloc = sz_rounded; /* the whole hog */
			}else{
				/* the extra we need to align by */
				to_alloc = sz_rounded - sz_initial;
			}

			if(fopt_mode & FOPT_VERBOSE_ASM){
				out_comment("stack alignment for %s (%u -> %u)",
						desc, stack_sz, stack_sz + sz_rounded);
				out_comment("alloc_n by %u (-> %u), padding with %u",
						sz_initial, stack_sz + sz_initial,
						sz_rounded - sz_initial);
			}

			v_stack_adj(to_alloc, 1);
		}

		stack_sz += sz_rounded;
	}

	return sz_rounded;
}

unsigned v_alloc_stack_n(unsigned sz, const char *desc)
{
	return v_alloc_stack2(sz, 1, desc);
}

unsigned v_alloc_stack(unsigned sz, const char *desc)
{
	return v_alloc_stack2(sz, 0, desc);
}

unsigned v_stack_align(unsigned const align, int force_mask)
{
	if(force_mask || (stack_sz & (align - 1))){
		type *const ty = type_nav_btype(cc1_type_nav, type_intptr_t);
		const unsigned new_sz = pack_to_align(stack_sz, align);
		const unsigned added = new_sz - stack_sz;

		v_push_sp();
		out_push_l(ty, added);
		out_op(op_minus);
		stack_sz = new_sz;

		if(force_mask){
			out_push_l(ty, align - 1);
			out_op(op_and);
		}
		out_flush_volatile();
		out_pop();
		out_comment("stack aligned to %u bytes", align);
		return added;
	}
	return 0;
}

void v_dealloc_stack(unsigned sz)
{
	/* callers should've snapshotted the stack previously
	 * and be calling us with said snapshot value
	 */
	UCC_ASSERT((sz & (cc1_mstack_align - 1)) == 0,
			"can't dealloc by a non-stack-align amount");

	v_stack_adj(sz, 0);

	stack_sz -= sz;
}
