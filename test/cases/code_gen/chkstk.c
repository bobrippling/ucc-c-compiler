// RUN: %ucc -S -o- %s -fno-pie -target x86_64-cygwin | %stdoutcheck --prefix=cygwin64 %s
// RUN: %ucc -S -o- %s -fno-pie -target i386-cygwin   | %stdoutcheck --prefix=cygwin32 %s

enum { PAGESIZE = 4096 };

void nocheck(void)
{
	// STDOUT-cygwin64:     nocheck:
	// STDOUT-NOT-cygwin64: /chkstk|alloca/
	// STDOUT-cygwin64:     ret

	// STDOUT-cygwin32:     nocheck:
	// STDOUT-NOT-cygwin32: /chkstk|alloca/
	// STDOUT-cygwin32:     ret
}

void yescheck(void)
{
	char buf[PAGESIZE];
	// STDOUT-cygwin64:       yescheck:
	// STDOUT-NEVER-cygwin64: alloca
	// STDOUT-cygwin64:       chkstk
	// STDOUT-cygwin64:       ret

	// STDOUT-cygwin32:       yescheck:
	// STDOUT-NEVER-cygwin32: chkstk
	// STDOUT-cygwin32:       alloca
	// STDOUT-cygwin32:       ret
}
