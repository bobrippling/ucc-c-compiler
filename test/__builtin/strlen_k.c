// RUN: %ocheck 8 %s

q(){}
char *s = "";

main()
{
	strlen(s);

	q(strlen("yo"));

	// shouldn't be const folded, since there's no terminating nul
	if(__builtin_constant_p(strlen((char[]){ 'a', 'b' }))){
		_Noreturn void abort();
		abort();
	}

	// 8
	return strlen("hezzos") + (__builtin_constant_p(strlen("hi")) ? 2 : 5);
}
