// RUN: %ucc -S -o %t %s -fpic -fno-leading-underscore

// RUN: grep -FA1 'movl i(%%rip), %%eax' %t | grep -F 'movl %%eax, %%edi'
// RUN: grep -FA2 'movq p(%%rip), %%rax' %t | grep -FA1 'movl (%%rax), %%eax' | grep -F 'movl %%eax, %%edi'
// RUN: grep -FA2 'movq defined_here@GOTPCREL(%%rip), %%rax' %t | grep -FA1 'movl (%%rax), %%eax' | grep -F 'movl %%eax, %%edi'
// RUN: grep -FA2 'movq elsewhere@GOTPCREL(%%rip), %%rax' %t | grep -FA1 'movl (%%rax), %%eax' | grep -F 'movl %%eax, %%edi'
// RUN: grep -FA2 'movl initialised_here(%%rip), %%eax' %t | grep -F 'movl %%eax, %%edi'

// RUN: grep -F 'leaq f(%%rip), %%rdi' %t
// RUN: grep -F 'movq g@GOTPCREL(%%rip), %%rdi' %t
// RUN: grep -F 'movq weak@GOTPCREL(%%rip), %%rdi' %t

// RUN: grep -FA1 'movq elsewhere@GOTPCREL(%%rip), %%rax' %t | grep -F 'movl $1, (%%rax)'
// RUN: grep -FA1 'movq defined_here@GOTPCREL(%%rip), %%rax' %t | grep -F 'movl $1, (%%rax)'
// RUN: grep -F   'movl $1, i(%%rip)' %t
// RUN: grep -F   'movl $1, initialised_here(%%rip)' %t

// RUN: grep -F 'movq elsewhere@GOTPCREL(%%rip), %%rdi' %t
// RUN: grep -F 'movq defined_here@GOTPCREL(%%rip), %%rsi' %t
// RUN: grep -F 'leaq i(%%rip), %%rdx' %t
// RUN: grep -F 'leaq initialised_here(%%rip), %%rcx' %t

void f(int i)
{
}

void g(int);

__attribute((weak))
void weak(int i)
{
}

void addr_func(void (int));
void addr_vars(int*,int*,int*,int*);

static int i;
static int *p = &i;

int defined_here;

extern int elsewhere;

int initialised_here = 3;

main()
{
	f(i);
	f(*p);
	f(defined_here);
	f(elsewhere);
	f(initialised_here);

	addr_func(f);
	addr_func(g);
	addr_func(weak);

	elsewhere = 1;
	defined_here = 1;
	i = 1;
	initialised_here = 1;

	addr_vars(&elsewhere, &defined_here, &i, &initialised_here);
}
