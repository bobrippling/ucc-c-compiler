// RUN: %ucc -o %t %s
// RUN: %ocheck 0 %t
// RUN: %t | %output_check 'Hello 5 2.3' 'Hello 5 2.3'

// should run without segfaulting
main()
{
	printf("Hello %d %.1f\n",
			5,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3); // this causes an infinite loop in glibc's printf()

	printf("Hello %d %.1f\n",
			5,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,
			2.3,  // this causes an infinite loop in glibc's printf()
			2.3); // and this causes a crash


	// suspect the bug is to do with double alignment when passed as stack arguments

	return 0;
}
