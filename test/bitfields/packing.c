// RUN: %ocheck 0 %s -DIMPL -DWITH_MAIN
// RUN: %layout_check %s

/*
int printf(const char *, ...)
	__attribute__((format(printf, 1, 2)));
*/

int memcmp(const void *, const void *, unsigned long);

typedef unsigned char uint8_t;
typedef unsigned uint32_t;

struct t1 {
	uint8_t op_type:1;
	uint8_t op_flags;
};

struct t2 {
	uint32_t op_type:1;
	uint8_t op_flags;
};

struct t3 {
	unsigned op_type:1;
	char op_flags;
};

struct t4 {
	int a:8;
	char rest[7];
};

struct t5 {
	int a:2;
	int b:3;
	int c:3;
	char rest[7];
};

struct t6 {
	unsigned op_type:1;
	struct {
		char op_flags;
	} a;
};

struct t7 {
	unsigned op_type:15;
	char op_flags;
};

struct rose_between_thorns {
	// this tests a non-bitfield between bitfields
	int x : 1,
		y,
		z : 1;
	// and also tests a bitfield at the end, after a non-bitfield
};

struct t1 t1 = { 1, 128 };
struct t2 t2 = { 1, 244 };
struct t3 t3 = { 0, 253 };
struct t4 t4 = { 134, { 9, 255, 251, 28, 3, 0, 123 } };
struct t5 t5 = { 0x1, 0x2, 0x3, { 9,9,9,9,9,9,9 } };
struct t6 t6 = { 0x1, { 0x2 } };
struct t7 t7 = { 52, 73 };
struct rose_between_thorns t8 = {
	.z = 1
};

_Static_assert(__alignof__(t1) == __alignof__(char), "");
_Static_assert(__alignof__(t2) == __alignof__(int), "");
_Static_assert(__alignof__(t3) == __alignof__(int), "");
_Static_assert(__alignof__(t4) == __alignof__(int), "");
_Static_assert(__alignof__(t5) == __alignof__(int), "");
_Static_assert(__alignof__(t6) == __alignof__(int), "");
_Static_assert(__alignof__(t7) == __alignof__(int), "");
_Static_assert(__alignof__(t8) == __alignof__(int), "");

extern int ec;

#ifdef IMPL
int ec;

void assert(_Bool b, int line)
{
	if(!b){
		printf("%s:%d: failed\n", __FILE__, line);
		ec = 1;
	}
}
#define assert(c) assert(c, __LINE__)
#endif

void check_t1(struct t1 *t, int a, int b)
#ifdef IMPL
{
	assert(t->op_type == a);
	assert(t->op_flags == b);
}
#else
;
#endif

void check_t2(struct t2 *t, int a, int b)
#ifdef IMPL
{
	assert(t->op_type == a);
	assert(t->op_flags == b);
}
#else
;
#endif

void check_t3(struct t3 *t, int a, int b)
#ifdef IMPL
{
	assert(t->op_type == a);
	assert(t->op_flags == b);
}
#else
;
#endif

void check_t4(struct t4 *t, int a, char b[static 7])
#ifdef IMPL
{
	assert(t->a == a);
	assert(memcmp(&t->rest, b, 7) == 0);
}
#else
;
#endif

void check_t5(struct t5 *t, int a, int b, int c, char rest[static 7])
#ifdef IMPL
{
	assert(t->a == a);
	assert(t->b == b);
	assert(t->c == c);
	assert(memcmp(&t->rest, rest, 7) == 0);
}
#else
;
#endif

void check_t6(struct t6 *t, int a, int b)
#ifdef IMPL
{
	assert(t->op_type == a);
	assert(t->a.op_flags == b);
}
#else
;
#endif

void check_t7(struct t7 *t, int a, int b)
#ifdef IMPL
{
	assert(t->op_type == a);
	assert(t->op_flags == b);
}
#else
;
#endif

void check_t8(struct rose_between_thorns *t, int x, int y, int z)
#ifdef IMPL
{
	assert(t->x == x);
	assert(t->y == y);
	assert(t->z == z);
}
#else
;
#endif

#if defined(WITH_MAIN)
int main()
{
	{
		struct t1 t1 = { 1, 128 };
		struct t2 t2 = { 1, 244 };
		struct t3 t3 = { 0, 253 };
		struct t4 t4 = { 134, { 9, 255, 251, 28, 3, 0, 123 } };
		struct t5 t5 = { 0x1, 0x2, 0x3, { 9,9,9,9,9,9,9 } };
		struct t6 t6 = { 0x1, { 0x2 } };
		struct t7 t7 = { 52, 73 };
		struct rose_between_thorns t8 = { .z = 1 };

		// check stack init / code gen
		check_t1(&t1, 1, 128);
		check_t2(&t2, 1, 244);
		check_t3(&t3, 0, (char)253);
		check_t4(&t4, (char)134, (char[7]){ 9, 255, 251, 28, 3, 0, 123 } );
		check_t5(&t5, 0x1, 0x2, 0x3, (char[7]){ 9,9,9,9,9,9,9 });
		check_t6(&t6, 1, 2);
		check_t7(&t7, 52, 73);
		check_t8(&t8, 0, 0, -1);
	}

	// check global ones / static layout gen
	check_t1(&t1, 1, 128);
	check_t2(&t2, 1, 244);
	check_t3(&t3, 0, (char)253);
	check_t4(&t4, (char)134, (char[7]){ 9, 255, 251, 28, 3, 0, 123 } );
	check_t5(&t5, 0x1, 0x2, 0x3, (char[7]){ 9,9,9,9,9,9,9 });
	check_t6(&t6, 1, 2);
	check_t7(&t7, 52, 73);
	check_t8(&t8, 0, 0, -1);

	return ec;
}
#endif
