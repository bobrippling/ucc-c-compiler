// RUN: %archgen %s 'x86,x86_64:movq %%rbx, -24(%%rbp)' 'x86,x86_64:movq %%r12, -16(%%rbp)' 'x86,x86_64:movq %%r13, -8(%%rbp)' 'x86,x86_64:movq -24(%%rbp), %%rbx' 'x86,x86_64:movq -16(%%rbp), %%r12' 'x86,x86_64:movq -8(%%rbp), %%r13'

main()
{
	f(a(), b(), c(), d());
}
