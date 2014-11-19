// RUN: %caret_check %s

g(int);

main()
{
'abc'; f(); "";
// CARETS:
//          ^ warning: unused expression \(string\)
//     ^ warning: implicit declaration of function "f"
}
