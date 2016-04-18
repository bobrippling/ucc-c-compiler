// RUN: %ucc -fsyntax-only %s -std=c89
// C89 for implicit function decl

typedef int tdef_int;

tdef_int i;

typedef tdef_int *ptr_to_tdef_int;

ptr_to_tdef_int p;

typedef ptr_to_tdef_int (*ptr_to_func_returning_ptr_to_int)();

ptr_to_func_returning_ptr_to_int f;

__typeof(f) *ptr_to_f_ret_ptr_to_int;

__typeof(1 + 2 * 3) an_int;

__typeof(q()) implicit_int;

// assert they are equal:
int i;
int *p;
int *(*f)();
int *(**ptr_to_f_ret_ptr_to_int)();
int an_int;
int implicit_int;
