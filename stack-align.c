main()
{
	printf("Hello %d %f\n",
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

	printf("Hello %d %f\n",
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
}
