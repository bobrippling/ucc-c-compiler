/* ucc: -fenable-asm */
#include <syscalls.h>

#define QUOTE(x) #x

main()
{
        __asm__(
                "mov rax, " QUOTE(SYS_exit) "\n"
                "mov rdi, 5\n"
                "syscall\n"
        );
        return 7;
}
