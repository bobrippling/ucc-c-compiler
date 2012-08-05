static void
out_asm(const char *fmt, ...)
{
	va_list l;
	putchar('\t');
	va_start(l, fmt);
	vprintf(fmt, l);
	va_end(l);
	putchar('\n');
}

void
out_func_prologue(int offset)
{
	out_asm("push %%rbp");
	out_asm("movq %%rsp, %%rbp");
	out_asm("subq %d, %%rsp", offset);
}

void
out_func_epilogue(void)
{
	out_asm("leaveq");
	out_asm("retq");
}

void
out_pop_func_ret(decl *d)
{
	(void)d;
	out_asm("popq %%rax");
}

void
out_store()
{
	struct vstack *store, *val;

	store = &vtop[0];
	val   = &vtop[-1];

	if(store->type == STACK){
		int r = gv(val);

		out_asm("mov_ %%%s, 0x%x(%%rbp)", r, store->bits.off_from_bp);
	}else{
		fprintf(stderr, "TODO: %s:%d\n", __FILE__, __LINE__);
		abort();
	}
}
