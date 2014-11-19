// RUN: %check %s -fgnu-keywords -w -pedantic -ffold-const-vlas

typedef int fn();

fn f // CHECK: warning: typedef function implementation is an extension
{
	return 3;
}

main()
{
	({ // CHECK: warning: use of GNU expression-statement
	});

	typeof(5) x; // CHECK: warning: use of GNU typeof()

	__attribute(()) char c; // CHECK: warning: use of GNU __attribute__

	int bunch[] = { [0 ... 1] = 1 }; // CHECK: warning: use of GNU array-range initialiser

	switch(bunch[0]){
		case 1 ... 3: // CHECK: warning: use of GNU case-range
			;
	}

	char ar[(0, 1)]; // CHECK: warning: comma-expr is a non-standard constant expression (for array size)

	char fn_name[] = __func__; // CHECK: warning: initialisation of char[] from __func__ is an extension

	struct A
	{
		int buf[]; // CHECK: warning: struct with just a flex-array is an extension
	};

	int; // CHECK: warning: declaration doesn't declare anything
}
