// RUN: %ucc -o %t %s
// RUN: %t | %stdoutcheck %s

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

main()
{
	int printf(const char *, ...);
	printf("%d\n", *(int *)&des);

	struct A st = { .y = 2, .x = 3 };
	printf("%d %d\n", st.x, st.y);

	printf("%d\n", 0xf & *(char *)&yo);
}
// STDOUT: 65536
// STDOUT-NEXT: -1 -2
// STDOUT-NEXT: 11
