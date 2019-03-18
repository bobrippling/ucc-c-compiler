// RUN: %check %s

enum E
{
	A = -1,
	B,
	C
};


void f(enum E e)
{
	switch(e){ // CHECK: !/warning: 'case [0-9]+' not a member of enum E/
		case A: // CHECK: !/warning: 'case [0-9]+' not a member of enum E/
		case B: // CHECK: !/warning: 'case [0-9]+' not a member of enum E/
		case C: // CHECK: !/warning: 'case [0-9]+' not a member of enum E/
			break;
	}

	// old behaviour:
	// checking switch usage - A == -1 is UINT_MAX,
	// so when we add one to it while checking we overflow,
	// and end up warning that every single number is not an enum member
}
