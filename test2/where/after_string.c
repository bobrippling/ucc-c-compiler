// RUN: %caret_check %s

f(int);

main()
{
"hello", f((void *)0);
// CARETS:
//         ^ note:
}
