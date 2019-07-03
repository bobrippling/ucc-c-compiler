// RUN: %ucc -S -o %t %s
//
// verify main is using callee save registers:
// RUN: awk '/main:/ { p = 1 } { if(p) print }' < %t > %t.maindisas
// RUN: grep 'mov.*%.bx' %t.maindisas >/dev/null
// RUN: rm -f %t.maindisas
//
// then run the test:
// RUN: %ocheck 0 %s
// RUN: %ocheck 0 %s -fstack-protector-all

long llabs(void)
{
	return 0;
}

void SNPRINTF(char *s, ...)
{
}

/* ---------------- original bug ---------------- */

int impl_val_str_r(void)
{
	long off = 0;
	SNPRINTF(0, 0, 0, 0, 0, 0, 0);
	SNPRINTF(0, off ? "-" : 0, llabs());
	return 1;
}
/*
 * stack:
 * 0-8: off
 * 8-16: %rbx AND stack-space for 7th arg AND saved space for 1st arg to 2nd SNPRINTF
 *
 * This bug is rare because it just so happens in this case that the stack is
 * perfectly filled by the call-space/spill values and the callee-save register stash.
 * Normally there would be 4-bytes or more, meaning the bug would go unnoticed.
 */


/* ---------------- attempt at causing fix to be buggy ---------------- */

int impl_val_str_r2(void)
{
	long off = 0;
	long off2 = 0; /* attempt to saturate the stack taking into account that it's now gone from 16-bytes to 32-bytes */
	SNPRINTF(0, 0, 0, 0, 0, 0, 0);
	SNPRINTF(0, off ? "-" : 0, llabs());
	return 1;
}

/* ---------------- harness for test below ---------------- */

void f(int g_, int i_, int h_)
{
	_Noreturn void abort(void);

	// this implicitly tests the callee save regs from main() weren't clobbered,
	// i.e. that impl_val_str_r() is able to save and restore them without them
	// being overwritten while on the stack
	if(g_ != -5) abort();
	if(h_ != -72) abort();

	if(i_ != 1) abort();
}

int g(void)
{
	return -5;
}

int h(void)
{
	return -72;
}

int main()
{
	// do something to cause us to require callee-save registers, so we can check
	// they're not clobbered by impl_val_str_r()'s save/restore of them
	f(g(), impl_val_str_r(), h());

	// check the second case
	f(g(), impl_val_str_r2(), h());

	// and again to try to aggravate
	f(g(), (impl_val_str_r2(), 1), h());
}
