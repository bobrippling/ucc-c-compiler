// RUN: %layout_check %s
typedef void (*sys_call_ptr_t)(void);

extern void sys_ni_syscall(void);

extern void sys_read(void);
#define __NR_read 0

#define __SYSCALL(nr, call) [nr] = (call),
#define __NR_syscall_max 299

const sys_call_ptr_t ent1[__NR_syscall_max+1] = {
	[0 ... __NR_syscall_max] = &sys_ni_syscall,

// FIXME: overrides the first, and so breaks the rest which depend on it
// FIXME: test middle array-range overrides
	__SYSCALL(__NR_read, sys_read)
};
