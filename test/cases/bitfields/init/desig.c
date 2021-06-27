// RUN: %ucc -o %t %s -std=c2x
// RUN: %t | %stdoutcheck %s
// RUN: %layout_check %s -DNOCODE

struct Desig
{
	int x : 16, y : 2;
} des = {
	.y = 1
};

struct A
{
	int x : 2, y : 2;
} yo = {
	.y = 2,
	.x = 3
};

#ifndef NOCODE
main()
{
	int printf(const char *, ...);
	_Static_assert(sizeof(des) == sizeof(int));
	printf("%d\n", *(int *)&des);

	struct A st = { .y = 2, .x = 3 };
	printf("%d %d\n", st.x, st.y);

	_Static_assert(sizeof(yo) == sizeof(char));
	printf("%d\n", 0xf & *(char *)&yo);
}
// STDOUT: 65536
// STDOUT-NEXT: -1 -2
// STDOUT-NEXT: 11
#endif
