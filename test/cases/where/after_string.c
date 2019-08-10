// RUN: %caret_check %s

f(int);

main()
{
"hello", f((void *)0);
// CARETS:
//         ^ warning: mismatching types, argument 1 to f
}
