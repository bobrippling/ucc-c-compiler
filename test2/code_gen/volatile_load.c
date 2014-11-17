// RUN: %archgen %s 'x86_64,x86:/movb -[0-9]*\(%%[er]bp\), %%al/' 'x86_64,x86:/movw \(%%[re]ax\), %%ax/' 'x86_64,x86:/mov[lq] -[0-9]*\(%[re]bp\), %[er]ax/'
// RUN: %check %s

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
