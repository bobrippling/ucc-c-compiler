// RUN: %ucc -target x86_64-linux %s -S -o %t -fno-pie -fstack-protector-all -fsanitize=undefined -fsanitize-error=call=abort2
//
// This checks that -fno-pie applies across the board - stackprotector, sanitize func, strings and labels.
//
// RUN: grep 'callq __stack_chk_fail$' %t
// RUN: grep 'callq abort2$' %t
// RUN: grep 'leaq str.1,' %t
// RUN: grep 'leaq .Lblk.10,' %t
//
// and again with PIC:
//
// RUN: %ucc -target x86_64-linux %s -S -o %t -fpie -fstack-protector-all -fsanitize=undefined -fsanitize-error=call=abort2
// RUN: grep 'callq __stack_chk_fail@PLT' %t
// RUN: grep 'callq abort2@PLT' %t
// RUN: grep 'leaq str.1(%%rip),' %t
// RUN: grep 'leaq .Lblk.10(%%rip),' %t

void f(const char *s, void *p)
{
	(void)s;
	(void)p;
}

int main()
{
	f("", &&a);

a:;

	int i = 0;
	i++;
}
