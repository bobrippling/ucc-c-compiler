// RUN: %ucc -S -o- -target x86_64-linux %s '-DADDR=0' -finline-functions -fno-semantic-interposition | grep 'movl $5, %%eax'
// RUN: %ucc -S -o- -target x86_64-linux %s '-DADDR=1' -finline-functions -fno-semantic-interposition | %stdoutcheck %s

//      STDOUT: movl $3, -4(%rbp)
// STDOUT-NEXT: movl -4(%rbp), %eax
//      STDOUT: addl $2, %eax

f(int i)
{
#if ADDR
	&i;
#endif
	return 2 + i;
}

caller()
{
	// '3' may be addressed
	return f(3);
}
