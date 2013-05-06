main()
{
	int x[2][2] /*= {
		{ 1, 2, },
		{ 3, 4, },
	}*/;

	return x[0][0] + x[0][1] + x[1][0] + x[1][1];
}

/*
http://stackoverflow.com/questions/1083488/dereferencing-pointer-to-integer-array
http://stackoverflow.com/questions/8391121/dereferencing-multi-dimensional-array-name-and-pointer-arithmetic
http://c-faq.com/~scs/cgi-bin/faqcat.cgi?sec=aryptr
http://stackoverflow.com/questions/1461432/what-is-array-decaying
 */
