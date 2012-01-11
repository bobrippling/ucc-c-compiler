#ifdef TIMMY
struct timmy
{
	int i;
	void *p;
};
#endif

int
main()
{
	struct int_struct_ptr
	{
		int i;
		struct ptr_struct
		{
			void *p;
		} sub;
		void *p;
	} x;
#ifdef TIMMY
	struct timmy tim; // FIXME
#endif

	x.sub;
}
