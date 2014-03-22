out_val *out_call(out_ctx *octx,
		out_val *fn, out_val **args,
		type *fnty)
{
	return impl_call(octx, fn, args, fnty);
}

void out_func_epilogue(out_ctx *octx, type *ty)
{
	impl_func_epilogue(octx, ty);

	octx->stack_local_offset = octx->stack_sz = 0;
}

void out_func_prologue(
		out_ctx *octx, const char *sp,
		type *rf,
		int stack_res, int nargs, int variadic,
		int arg_offsets[], int *local_offset)
{
	UCC_ASSERT(octx->stack_sz == 0, "non-empty stack for new func");

	impl_func_prologue_save_fp();

	if(mopt_mode & MOPT_STACK_REALIGN)
		v_stack_align(octx, cc1_mstack_align, 1);

	impl_func_prologue_save_call_regs(rf, nargs, arg_offsets);

	if(variadic) /* save variadic call registers */
		impl_func_prologue_save_variadic(rf);

	/* setup "pointers" to the right place in the stack */
	stack_variadic_offset = stack_sz - platform_word_size();
	stack_local_offset = stack_sz;
	*local_offset = stack_local_offset;

	if(stack_res)
		v_alloc_stack(stack_res, "local variables");
}
