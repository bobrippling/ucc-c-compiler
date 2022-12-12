// check runtime and static-gen:
// RUN: %ocheck 0 %s
// RUN: %layout_check %s
// RUN: %check %s -Wbitfield-boundary

void *memset(void *, int, unsigned long);

main()
{
#include "../ocheck-init.c"
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

// -------------

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

struct data
{
	unsigned int foo:4;
	unsigned int bar:4; // 1
	uint8_t other; // 2
	uint16_t other2; // 4
};

_Static_assert(sizeof(struct data) == 4);

struct Basic
{
	int x : 4, y : 4;
} bas = { 1, 2 };

_Static_assert(sizeof(bas) == sizeof(int));

struct Basic2
{
	char x : 4, y : 4;
} bas2 = { 1, 2 };

_Static_assert(sizeof(bas2) == sizeof(char));

struct Another
{
    short s;      // 0
    char c;       // 2
    int flip:1;   // 3, 0-0
    int nybble:4; // 3, 1-4
    int septet:7; // 4: 0-6 - we only have 1 byte of space above, this overflows
} another = {
	1, 2, 3, 4, 5,
};

_Static_assert(sizeof(another) == 8);
