// RUN: %ocheck 0 %s
// RUN: %check %s -Wbitfield-boundary

void *memset(void *, int, unsigned long);

main()
{
	struct
	{
		int a : 16;
		// should pad here int:16
		int b : 19; // CHECK: /warning: bitfield overflow \(16 \+ 19 > 32\) - moved to next boundary/
		int c;
	} s;

	memset(&s, 0, sizeof s);

	s.a = 2; // loads *s
	s.b = 3; // loads 1[(int *)s]
	s.c = 4; // loads 2[(int *)s]

	_Static_assert(sizeof(s) == 3 * sizeof(int), "");

	// ensure we pad to 3 ints
	if(0[(int *)&s] != 2) return 1;
	if(1[(int *)&s] != 3) return 2;
	if(2[(int *)&s] != 4) return 3;

	return 0;
}
