// RUN: %ocheck 0 %s
// RUN: %ucc -target x86_64-linux -S -o- %s | grep 'addl \$2,'

void abort(void) __attribute__((noreturn));

main()
{
	char c = 3;

	c += 2;

	if(c != 5)
		abort();

	return 0;
}
