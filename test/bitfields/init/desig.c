// RUN: %ocheck 0 %s

struct Desig
{
	int x : 16, y : 2;
} des = {
	.y = 1
};
// 00000000 00000000
// ------01 --------
// (little endian)

struct A
{
	int x : 2, y : 2;
} yo = {
	.y = 2,
	.x = 3
};
// ----1110 --------
// -------- --------
// (little endian)

#ifdef IMPL
static void assert(long b)
{
	if(b)
		return;
	__attribute((noreturn)) void abort(void);
	abort();
}

int memcmp(const void *, const void *, unsigned long);

int main()
{
	char cmp[] = { 0, 0, 1, 0 };
	_Static_assert(sizeof(cmp) == sizeof(des), "");
	assert(!memcmp(cmp, &des, sizeof(cmp)));

	struct A st = { .y = 2, .x = 3 };

	assert(st.x == -1);
	assert(st.y == -2);

	_Static_assert(sizeof(yo) == sizeof(int), "");
	assert((0xf & *(char *)&yo) == 11);
}
#endif
