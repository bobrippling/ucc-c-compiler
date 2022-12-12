// RUN: %check %s -Womitted-param-types -Wimplicit-old-func -DSYNTAX_ONLY -DOLD_FUNCS
// RUN: %ocheck 2 %s -DOLD_FUNCS
// RUN: %check --only --prefix=C2X -e %s -std=c2x -DC2X_TEST

#ifdef SYNTAX_ONLY
two(a, b) // CHECK: warning: old-style function declaration
	int a, b; // here we can get information for a warning below; two()
{
}

empty(); // CHECK: warning: old-style function declaration (needs "(void)")

syntax_only()
{
	two(two(), two()); // CHECK: warning: too few arguments to function two (got 0, need 2)
	empty(empty(), empty()); // CHECK: !/warn/
}
#endif

#ifdef OLD_FUNCS
single(hello); // CHECK: warning: old-style function declaration
// int hello

single2(a) // CHECK: warning: old-style function declaration
{
	return a;
}

main()
{
#include "../ocheck-init.c"
	return single(5) + single2(1); // 2
}

single(a) // CHECK: warning: old-style function declaration
	int a; // matches with int hello
{
	return a ? 1 : 0;
}
#endif

#ifdef C2X_TEST
void empty();

int main()
{
#include "../ocheck-init.c"
	empty(1, 2); // CHECK-C2X: error: too many arguments to function empty (got 2, need 0)
}
#endif
