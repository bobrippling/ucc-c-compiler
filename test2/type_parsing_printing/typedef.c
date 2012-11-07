typedef int tdef_int;

tdef_int i;

typedef tdef_int *ptr_to_tdef_int;

ptr_to_tdef_int p;

typedef ptr_to_tdef_int (*func_returning_ptr_to_int)();

func_returning_ptr_to_int f;
