// RUN: %check %s

// pointer to function
int (__attribute__((__ucc_debug)) *attributed_ptr_fn)(long y); // CHECK: debug attribute handled
int attributed_fn_ptr_arg(int (__attribute__((__ucc_debug)) *attributed_ptr_fn_arg)()); // CHECK: debug attribute handled

void (*fn_attributed_fn_arg)(void (*)(__attribute__((__ucc_debug)) x)); // CHECK: debug attribute handled
void (*fn_attributed_fn_arg)(void (*)(__attribute__((__ucc_debug))));

// function itself
int (__attribute((__ucc_debug)) attributed_fn)(); // CHECK: debug attribute handled

// second decl in a list
int first,                               // CHECK: !/debug/
		__attribute__((__ucc_debug)) second, // CHECK: debug attribute handled
		third;                               // CHECK: !/debug/

void __attribute__((__ucc_debug)) first_fn(void), // CHECK: debug attribute handled
		 __attribute__((__ucc_debug)) second_fn(void); // CHECK: debug attribute handled

void first_fn_suff(void) __attribute__((__ucc_debug)), // CHECK: debug attribute handled
		 second_fn_suff(void) __attribute__((__ucc_debug)); // CHECK: debug attribute handled
