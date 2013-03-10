typedef void (*sys_call_ptr_t)(void);

extern void sys_ni_syscall(void);

extern void sys_read(void);
#define __NR_read 0

#define __SYSCALL(nr, call) [nr] = (call),
#define __NR_syscall_max 299

const sys_call_ptr_t sys_call_table[__NR_syscall_max+1] = {
	[0 ... __NR_syscall_max] = &sys_ni_syscall,

	__SYSCALL(__NR_read, sys_read)
};
