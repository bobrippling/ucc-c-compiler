main()
{
	struct
	{
	} a;

	if(0 != sizeof a)
		abort();
}
