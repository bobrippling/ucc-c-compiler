// RUN: %ucc -S -o %t %s -fpic -fno-leading-underscore -fno-semantic-interposition

// RUN: grep -FA1 'movl i(%%rip), %%eax' %t | grep -F 'movl %%eax, %%edi'
// RUN: grep -FA2 'movq p(%%rip), %%rax' %t | grep -FA1 'movl (%%rax), %%eax' | grep -F 'movl %%eax, %%edi'
// RUN: grep -FA2 'movq defined_here@GOTPCREL(%%rip), %%rax' %t | grep -FA1 'movl (%%rax), %%eax' | grep -F 'movl %%eax, %%edi'
// RUN: grep -FA2 'movq elsewhere@GOTPCREL(%%rip), %%rax' %t | grep -FA1 'movl (%%rax), %%eax' | grep -F 'movl %%eax, %%edi'
// RUN: grep -FA3 'movq initialised_here@GOTPCREL(%%rip), %%rax' %t | grep -FA2 'movl (%%rax), %%eax' | grep -F 'movl %%eax, %%edi'
// RUN: grep -FA2 'movl elsewhere_hidden(%%rip), %%eax' %t | grep -F 'movl %%eax, %%edi'

// RUN: grep -F 'movq func_here@GOTPCREL(%%rip), %%rdi' %t
// RUN: grep -F 'movq func_elsewhere@GOTPCREL(%%rip), %%rdi' %t
// RUN: grep -F 'movq weak@GOTPCREL(%%rip), %%rdi' %t
// RUN: grep -F 'leaq func_hidden(%%rip), %%rdi' %t

// RUN: grep -FA1 'movq elsewhere@GOTPCREL(%%rip), %%rax' %t | grep -F 'movl $1, (%%rax)'
// RUN: grep -FA1 'movq defined_here@GOTPCREL(%%rip), %%rax' %t | grep -F 'movl $1, (%%rax)'
// RUN: grep -F   'movl $1, i(%%rip)' %t
// RUN: grep -F   'movq initialised_here@GOTPCREL(%%rip), %%rax' %t
// RUN: grep -F   'movl $1, elsewhere_hidden(%%rip)' %t

// RUN: grep -F 'movq elsewhere@GOTPCREL(%%rip), %%rdi' %t
// RUN: grep -F 'movq defined_here@GOTPCREL(%%rip), %%rsi' %t
// RUN: grep -F 'leaq i(%%rip), %%rdx' %t
// RUN: grep -F 'movq initialised_here@GOTPCREL(%%rip), %%rcx' %t
// RUN: grep -F 'leaq elsewhere_hidden(%%rip), %%r8' %t

void func_here(int i)
{
}

void func_elsewhere(int);

void func_hidden(int) __attribute__((visibility("hidden")));

__attribute((weak))
void weak(int i)
{
}

void addr_func(void (int));
void addr_vars(int*,int*,int*,int*,int*);

static int i;
static int *p = &i;

int defined_here;

extern int elsewhere;

int initialised_here = 3;

extern int elsewhere_hidden __attribute__((visibility("hidden")));

main()
{
	func_here(i);
	func_here(*p);
	func_here(defined_here);
	func_here(elsewhere);
	func_here(initialised_here);
	func_here(elsewhere_hidden);

	addr_func(func_here);
	addr_func(func_elsewhere);
	addr_func(weak);
	addr_func(func_hidden);

	elsewhere = 1;
	defined_here = 1;
	i = 1;
	initialised_here = 1;
	elsewhere_hidden = 1;

	addr_vars(&elsewhere, &defined_here, &i, &initialised_here, &elsewhere_hidden);
}
