// RUN: %ocheck 0 %s
// RUN: ! %ucc -target x86_64-linux -S -o- %s | grep addb

void abort(void) __attribute__((noreturn));

main()
{
	for(unsigned char c = 1; c > 0; c++)
		;

	char c = 3;

	c += (int)2; // this should generate addl, not addb
	// i.e. integer promotion rules (for compound assign)

	if(c != 5)
		abort();

	return 0;
}
