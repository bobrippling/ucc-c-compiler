// RUN: %ocheck 0 %s

main()
{
	union
	{
		struct
		{
			int st_first;
			int st_second;
		} st;
	} x;

	x.st.st_first = 5;

	if(*(int *)((char *)&x.st.st_second - sizeof(int)) != 5)
		abort();

	if(5 != (*(int *)((char *)&x.st.st_second - sizeof(int))))
		abort();

	return 0;
}
