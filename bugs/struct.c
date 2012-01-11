int
main()
{
	struct int_struct_ptr
	{
		int i;
		struct one_int
		{
			int j;
		} sub;
		void *p;
	} x;

	//nam.mem;

	x.i;

	x.sub.j;

	//nam.mem.mem;
}
