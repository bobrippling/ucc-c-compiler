// RUN: ! %ucc -S -o- %s -DN=7                                                   | grep __stack_chk_fail
// RUN: ! %ucc -S -o- %s -DN=7 -fstack-protector                                 | grep __stack_chk_fail
// RUN:   %ucc -S -o- %s -DN=7 -fstack-protector-all                             | grep __stack_chk_fail
// RUN:   %ucc -S -o- %s -DN=7 -fstack-protector     -DATTR='stack_protect'      | grep __stack_chk_fail
// RUN: ! %ucc -S -o- %s -DN=7 -fstack-protector-all -DATTR='no_stack_protector' | grep __stack_chk_fail
// RUN:   %ucc -S -o- %s -DN=8 -fstack-protector                                 | grep __stack_chk_fail
//
// RUN: %ucc -o %t %s -fstack-protector-all -DN=8 -DMAIN
// RUN: %t 1234567
// RUN: %ocheck SIGABRT %t 123456789AAAAAAAAA

#ifndef ATTR
#define ATTR
#endif

char *strcpy(char *, const char *);

__attribute((ATTR))
int f(char *s)
{
	char buf[N];
	strcpy(buf, s);
	return 0;
}

#ifdef MAIN
int main(int argc, char **argv)
{
	return f(argv[1]);
}
#endif
