int  ((*p)(void)); // ptr  -> func -> int
int *(( f)(void));   // func -> ptr  -> int

int *g();

int (*(*ptr_to_f_ret_ptr_to_f)(void))(void);
