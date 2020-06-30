//#include <unistd.h>
//#include <sys/syscall.h>

#define SYSCALL_CLOBBERS "rcx", "r11"

void start() //asm("start")
{
	//register int    syscall_no  asm("rax") = 1;
	//register int    arg1        asm("rdi") = 1;
	//register char*  arg2        asm("rsi") = "hello, world!\n";
	//register int    arg3        asm("rdx") = 14;

	asm(
			"syscall"
			:
			: "a"(33554436)
			, "D"(1)
			, "S"("hello\n")
			, "d"(6)
			: SYSCALL_CLOBBERS
	);

	asm(
			//"movl $33554433,%eax;"
			//"xorl %ebx,%ebx;"
			"syscall"
			:
			: "a"(33554433)
			, "D"(61)
			: SYSCALL_CLOBBERS
		 );
}

