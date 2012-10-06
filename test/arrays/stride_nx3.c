f();

main()
{
	extern int a[][3];

	f(
		a    // a
		[1]  // + 3 * (1 * 4)         // array_size * (index * sizeof(int))
		[2]  // +     (2 * 4)         //     1        (index * sizeof(int))

	       // = a + 12 + 8 = a + 20
	 );


	f(
		a    // a
		[2]  // + 3 * (2 * 4)         // array_size * (index * sizeof(int))
		[3]  // +     (3 * 4)         //     1        (index * sizeof(int))

	       // = a + 24 + 12 = a + 36
	 );
}
