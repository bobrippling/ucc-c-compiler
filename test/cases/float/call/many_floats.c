// RUN: %ucc -o %t %s
// RUN: %ocheck 0 %t
// RUN: %t | %stdoutcheck %s
// STDOUT: Hello 5 5.9
// STDOUT-NEXT: 7.3 8.7 10.1 11.5 12.9 14.3 15.7 17.1 18.5 19.9
// STDOUT-NEXT: Hello 5 15.7
// STDOUT-NEXT: 14.3 12.9 11.5 10.1 8.7 7.3 5.9 4.5 3.1 1.7 0.3

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

main()
{
	printf("Hello %d %.1f\n"
			"%.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f\n",
			5,
			5.9,
			7.3,
			8.7,
			10.1,
			11.5,
			12.9,
			14.3,
			15.7,
			17.1,
			18.5,
			19.9); // this causes an infinite loop in glibc's printf()

	printf("Hello %d %.1f\n"
			"%.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f\n",
			5,
			15.7,
			14.3,
			12.9,
			11.5,
			10.1,
			8.7,
			7.3,
			5.9,
			4.5,
			3.1,
			1.7, // this causes an infinite loop in glibc's printf()
			0.3); // and this causes a crash


	// suspect the bug is to do with double alignment when passed as stack arguments

	return 0;
}
