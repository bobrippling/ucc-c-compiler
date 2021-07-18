// RUN: %check %s -DTYPES

// RUN: %ucc -target x86_64-linux -S -o- %s -DTYPES | %stdoutcheck --prefix=types %s
// STDOUT-types: /movb -[0-9]*\(%[er]bp\), %al/
// STDOUT-types: /movw \(%[re]ax\), %ax/
// STDOUT-types: /mov[lq] -[0-9]*\(%[re]bp\), %[er]ax/

// RUN: %ucc -target x86_64-linux -S -o- %s           | grep 'movl -[0-9]*(%%[er]bp), %%eax'
// RUN: %ucc -target x86_64-linux -S -o- %s -DTO_VOID | grep 'movl -[0-9]*(%%[er]bp), %%eax'

#ifdef TYPES
f(volatile char x)
{
	x; // CHECK: !/warning: unused expression/
}

g(volatile short *p)
{
	*p; // CHECK: !/warning: unused expression/
}

h(volatile void *v)
{
	*v; // CHECK: !/warning: unused expression/
}
#else
f(volatile int x)
{
#ifdef TO_VOID
	(void)
#endif
	x;
}
#endif
