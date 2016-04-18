// RUN: %archgen %s 'x86,x86_64:/movq %%rbx, -[0-9]+\(%%rbp\)/' 'x86,x86_64:/movq %%r12, -[0-9]+\(%%rbp\)/' 'x86,x86_64:/movq %%r13, -[0-9]+\(%%rbp\)/' 'x86,x86_64:/movq -[0-9]+\(%%rbp\), %%rbx/' 'x86,x86_64:/movq -[0-9]+\(%%rbp\), %%r12/' 'x86,x86_64:/movq -[0-9]+\(%%rbp\), %%r13/'

typedef int fn(void);

fn a, b, c, d;

void f(int, int, int, int);

main()
{
	f(a(), b(), c(), d());
}
