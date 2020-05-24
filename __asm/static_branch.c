#define add_random_kstack_offset() do {					\
	if (static_branch_maybe(CONFIG_RANDOMIZE_KSTACK_OFFSET_DEFAULT,	\
				&randomize_kstack_offset)) {		\
		u32 offset = this_cpu_read(kstack_offset);		\
		char *ptr = __builtin_alloca(offset & 0x3FF);		\
		asm volatile("" : "=m"(*ptr));				\
	}								\
} while (0)

#define static_branch_maybe(...) x
int x;
typedef unsigned u32;
int kstack_offset;
u32 this_cpu_read(int);

main() {
	add_random_kstack_offset();
}
