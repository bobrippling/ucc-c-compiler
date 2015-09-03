int printf(const char *, ...);
void *malloc(unsigned long);
void free(void *);

void check_ptr(int *p)
{
	long l;

#ifdef __UCC__
	l = (long)p;
#else
	__asm("" : "=g"(l) : "0"(p));
#endif

	//printf("l = %#lx\n", l);

	volatile const char *const rax = (void *)0x100000000000; // movabsq	$17592186044416, %rax
	long rdx = l; // movq	%rdi, %rdx
	rdx >>= 3; // shrq	$3, %rdx
	const long dl = rax[rdx]; //movb	(%rdx,%rax), %dl

	int eax = l; // movq	%rdi, %rax
	eax &= 7; //andl	$7, %eax
	eax += 3; //addl	$3, %eax

	if(eax >= dl && dl){
		_Noreturn void __asan_report_load4(void *);
		__asan_report_load4(p);
		printf("%#lx is bad\n", l);
		return;
	}

	printf("%#lx is fine\n", l);
}

__attribute((constructor))
static void asan_init(void)
{
	void __asan_init_v4(void);
	__asan_init_v4();
}

void free_(void *pp)
{
	free(*(void **)pp);
}

int main()
{
	int *p __attribute((cleanup(free_))) = malloc(10);

	check_ptr(p); // good
	check_ptr(5); // bad
	check_ptr(p - 1); // bad
	check_ptr(p + 1); // good
	check_ptr(p + 2); // bad
}
