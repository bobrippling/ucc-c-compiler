// RUN: %ocheck 0 %s

typedef struct A A;

enum { false, true };

// 0 or 1, depending on whether Y is a null pointer constant
#define IS_NULL_PTR_CONST(Y)        \
_Generic((1 ? (A *)0 : (void *)(Y)), \
         A *: true,                 \
         default: false)

#define MAYBE_EVAL(Y)                     \
_Generic((1 ? (A *)0 : (void *)((Y)-(Y))), \
         A *: 0,                          \
         default: f(Y))

#define NULL (void *)0

true_null_ptr_1 = IS_NULL_PTR_CONST(NULL);
true_null_ptr_2 = IS_NULL_PTR_CONST(0);
false_null_ptr = IS_NULL_PTR_CONST(1);

//dont_eval = MAYBE_EVAL(f());

f()
{
	return 3;
}

void abort();

main()
{
	int do_eval = MAYBE_EVAL(f());

	if(do_eval != 3)
		abort();

	if(!true_null_ptr_1)
		abort();
	if(!true_null_ptr_2)
		abort();

	if(false_null_ptr)
		abort();

	return 0;
}
