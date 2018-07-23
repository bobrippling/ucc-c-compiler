#ifdef COMPLEX
int printf();

// stdcall is not important, can be any attribute
#define ATTR __attribute__((__stdcall__))
// problem does not occur if there are no attributes
// #define ATTR

int ATTR actual_function() {
  return 42;
}

void* function_pointer;

int main() {
    function_pointer = &actual_function;

    // compiles correctly
    int a = ((ATTR int(*) (void)) function_pointer)();

    // => 42
    printf("%i\n", a);

    // tcc thinks the result of this expression is a pointer
    // "warning: assignment makes integer from pointer without a cast"
    int b = ( (int(ATTR *)(void))  function_pointer)();

    // compilation continues, but binary
    // crashes at runtime before it reaches this line
    printf("%i\n", b);

    return 0;
}
#elif DECL

__attribute__((__ucc_debug)) int (*pf0)(void);
int (*__attribute__((__ucc_debug)) pf1)(void);
int (__attribute__((__ucc_debug)) *pf2)(void);

#else

main()
{
	//(__attribute__((__ucc_debug)) int (*)(void))0;
	//(int (*__attribute__((__ucc_debug)) )(void))0;
	//(int (__attribute__((__ucc_debug)) *)(void))0;

	(int (__attribute__(()) int))0; // function
	(int (__attribute__(()) *))0;   // int*
	(int (__attribute__(()) *)())0; // int (*)()
}

#endif
