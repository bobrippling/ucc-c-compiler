void g()
{
	typedef struct A *P;

	struct A
	{
		void (*f)(P cinfo);
	};

	P cinfo = 0;

	(*cinfo->f)(cinfo);
}
