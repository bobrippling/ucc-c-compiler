// RUN: %ucc -fsyntax-only %s

int  ((*p)(void));
int *(( f)(void));

int *g();

int (*(*ptr_to_f_ret_ptr_to_f)(void))(void);

main()
{
	// 1- checks it's an int, not a pointer
	1 - *g();
	1 - (*(*ptr_to_f_ret_ptr_to_f)())();
	1 - (*p)();
	1 - *(f());
}
