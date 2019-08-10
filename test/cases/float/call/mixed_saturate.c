// RUN: %ucc -o %t %s
// RUN: %check --only %s
// RUN: %t | %stdoutcheck %s
// STDOUT: 1
// STDOUT-NEXT: 2
// STDOUT-NEXT: 3
// STDOUT-NEXT: 4
// STDOUT-NEXT: 5
// STDOUT-NEXT: 6
// STDOUT-NEXT: 7
// STDOUT-NEXT: 8
// STDOUT-NEXT: 9
// STDOUT-NEXT: 10
// STDOUT-NEXT: 11
// STDOUT-NEXT: 12
// STDOUT-NEXT: 13
// STDOUT-NEXT: 14
// STDOUT-NEXT: 15
// STDOUT-NEXT: 16
// STDOUT-NEXT: 17
// STDOUT-NEXT: 18

extern printf(const char *, ...) __attribute__((format(printf, 1, 2)));

void f(
		int i1, float f1,
		int i2, float f2,
		int i3, float f3,
		int i4, float f4,
		int i5, float f5,
		int i6, float f6,
		int i7, float f7,
		int i8, float f8,
		int i9, float f9
		// 3 stack ints, 1 stack float
 )
{
	printf(
			"%d\n%.0f\n"
			"%d\n%.0f\n"
			"%d\n%.0f\n"
			"%d\n%.0f\n"
			"%d\n%.0f\n"
			"%d\n%.0f\n"
			"%d\n%.0f\n"
			"%d\n%.0f\n"
			"%d\n%.0f\n"
			,
		i1, f1,
		i2, f2,
		i3, f3,
		i4, f4,
		i5, f5,
		i6, f6,
		i7, f7,
		i8, f8,
		i9, f9);
}

main()
{
	f(1, 2, 3, 4, 5, 6, 7, 8,
	  9, 10, 11, 12, 13, 14, 15, 16,
		17, 18);
}
