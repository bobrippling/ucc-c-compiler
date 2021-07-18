// RUN: %debug_scope %s -c -fno-semantic-interposition

void f_indirect(void); // this would call 'extern int f()'

__attribute((noinline))
static int g()
{
	return 7;
}

__attribute((always_inline))
inline int f(int);

main()
{
	f_indirect();
	return f(1);
}

// the inlining of f() in main() will have caused
// a DW_TAG_subprogram to be created for the inline
// instance.
//
// need to make sure we correctly reuse this tag when
// emitting proper debug information for 'extern int f()'

int f(int x)
{ //SCOPE: x i
	int i = x + 1; //SCOPE: x i
	return i - g(); //SCOPE: x i
}
