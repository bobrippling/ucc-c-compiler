// RUN: %ucc -fno-const-fold -funsigned-char %s -o %t
// RUN: %t
void abort(void) __attribute__((noreturn));

main()
{
	int is_unsigned = (int)(char)-1 == 255;

	if(!is_unsigned)
		abort();

	return 0;
}
