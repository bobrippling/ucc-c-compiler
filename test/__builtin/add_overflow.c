// RUN: %ocheck 0 %s -ftrapv

static void assert(_Bool b, int line, const char *s)
{
	if(!b){
		//_Noreturn void abort(void);
		//abort();
		printf(__FILE__ ":%d: %s\n", line, s);
	}
}
#define assert(b) assert(b, __LINE__, #b)

static int overflow_long(short x, char y)
{
	// longest type is the output
	long z;
	return __builtin_add_overflow(x, y, &z);
}

static int overflow_short(short x, char y)
{
	// longest type is input 1
	short z;
	return __builtin_add_overflow(x, y, &z);
}

static int overflow_int(int x, char y)
{
	// longest type is input 2 (and output)
	int z;
	return __builtin_add_overflow(x, y, &z);
}

static int overflow_char(short x, char y)
{
	// longest type is input 1, but must truncate to char (output)
	char z;
	return __builtin_add_overflow(x, y, &z);
}

static int underflow_int(int a, int b)
{
	int r;
	return __builtin_sub_overflow(a, b, &r);
}

static int multiply_int(int a, int b)
{
	int r;
	return __builtin_mul_overflow(a, b, &r);
}

_Static_assert(_Generic(__builtin_add_overflow(1, 2, &(int){0}), int: 1, _Bool: 2) == 2, "");

int main()
{
	long longmax = ~0ul / 2;
	int intmax = ~0u / 2;
	int intmin = -intmax - 1;
	short shortmax = 32767;
	signed char charmax = 127;

	assert(overflow_long(3, 2) == 0);
	assert(overflow_long(longmax, 1) == 0);

	assert(overflow_char(shortmax, 0) == 1); // ucc
	assert(overflow_char(charmax, 0) == 0);
	assert(overflow_char(charmax, 1) == 1); // ucc

	assert(overflow_short(shortmax, 0) == 0);
	assert(overflow_short(shortmax, 1) == 1);
	assert(overflow_short(shortmax - 1, 1) == 0);

	assert(overflow_int(intmax, 0) == 0);
	assert(overflow_int(intmax, 1) == 1);
	assert(overflow_int(intmax - 1, 1) == 0);
	assert(overflow_int(-intmax, 0) == 0);
	assert(overflow_int(-intmax, 1) == 0);
	assert(overflow_int(-intmax, -1) == 0);
	assert(overflow_int(-intmax, -2) == 1);

	assert(underflow_int(intmin, 1) == 1);
	assert(underflow_int(intmin, 0) == 0);
	assert(underflow_int(intmin, -1) == 0);

	assert(multiply_int(intmax, 1) == 0);
	assert(multiply_int(intmax, 2) == 1);
	assert(multiply_int(intmax / 2, 2) == 0);
	assert(multiply_int(intmax / 2 + 1, 2) == 1);
}
